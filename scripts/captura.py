"""
captura.py — Captura e visualiza dados de teste do ESP32 via Serial.

Este script se conecta a um ESP32, envia um comando para iniciar um teste de
motor em malha fechada, captura a telemetria em formato CSV e, ao final,
salva os dados e gera um gráfico da resposta.

Uso:
  python captura.py [porta] [--rpm <valor>] [--dur <segundos>]

Exemplos:
  python captura.py                # Auto-detecta a porta e inicia o teste.
  python captura.py COM5           # Usa a porta COM5
  python captura.py --rpm 150      # Define o alvo para 150 RPM.
  python captura.py --dur 5        # Define a duração para 5 segundos.
  python captura.py --rpm 300 --dur 10 # Define alvo e duração.
"""

import serial
import serial.tools.list_ports
import struct
import sys
import time
import csv
import matplotlib.pyplot as plt

# --- Configuração ---
BAUD_RATE = 115200  # Deve ser igual ao `Serial.begin()` no ESP32

# VIDs/PIDs comuns de placas ESP32
ESP32_USB_IDS = [
    (0x10C4, 0xEA60),  # CP2102 (ESP32 DevKit V1)
    (0x1A86, 0x7523),  # CH340
    (0x303A, 0x1001),  # USB-JTAG nativo do ESP32-S2/C3
]


def encontrar_porta():
    """Detecta automaticamente a porta serial do ESP32 pelo VID:PID."""
    portas = serial.tools.list_ports.comports()

    for porta in portas:
        if porta.vid and porta.pid:
            for vid, pid in ESP32_USB_IDS:
                if porta.vid == vid and porta.pid == pid:
                    return porta.device

    # Fallback: retorna a primeira porta disponível (se houver apenas uma)
    if len(portas) == 1:
        return portas[0].device

    return None


def executar_teste(porta, target_rpm, duration_s):
    """Executa o teste e retorna os dados capturados."""
    print(f"Conectando a {porta} a {BAUD_RATE} baud...")
    ser = serial.Serial(porta, BAUD_RATE, timeout=5)
    
    # IMPORTANTE: Evita que o ESP32 trave no bootloader (problema comum no Windows)
    ser.setDTR(False)
    ser.setRTS(False)
    
    print("Aguardando o ESP32 inicializar...")
    tempo_inicio = time.time()
    pronto = False
    
    # Ouve a porta serial até o ESP32 imprimir a mensagem do setup()
    while time.time() - tempo_inicio < 5:
        try:
            linha = ser.readline().decode('utf-8', errors='ignore').strip()
            if linha:
                print(f"[ESP32] {linha}") # Mostra o que a placa está a dizer
            
            # Quando a placa pedir a letra 'T', sabemos que está pronta!
            if "Envie a letra 'T'" in linha:
                pronto = True
                break
        except Exception:
            pass
            
    if not pronto:
        print("Aviso: O ESP32 demorou a responder, mas vamos tentar enviar o comando mesmo assim.")
        
    time.sleep(0.5) # Pausa curta para estabilizar os buffers
    
    # Limpa buffer de entrada
    ser.reset_input_buffer()
    
    # Envia o comando para iniciar o teste
    duration_ms = int(duration_s * 1000)
    cmd = f"T:{target_rpm},{duration_ms}\n"
    print(f"Enviando comando para iniciar o teste: RPM={target_rpm}, Duração={duration_s}s...")
    ser.write(cmd.encode())

    dados = []
    capturando = False
    
    print("Aguardando início do teste...")
    print("Pressione Ctrl+C para interromper.")

    while True:
        try:
            linha = ser.readline().decode('utf-8').strip()
        except UnicodeDecodeError:
            continue

        if not linha:
            print("Timeout na leitura. Nenhum dado recebido.")
            break

        if "INICIO DO TESTE" in linha:
            capturando = True
            print("Captura iniciada...")
            # Pula a linha do cabeçalho CSV
            ser.readline() 
            continue

        if "FIM DO TESTE" in linha:
            print("Fim do teste recebido.")
            capturando = False
            break

        if capturando:
            try:
                # A linha é "Tempo(ms),Setpoint,RPM,PWM"
                tempo_ms, setpoint, rpm, pwm = map(float, linha.split(','))
                dados.append((tempo_ms, setpoint, rpm, int(pwm)))
                if len(dados) % 25 == 0: # Imprime progresso
                    print(f"  {len(dados)} amostras capturadas...", end='\r')
            except (ValueError, IndexError):
                pass  # Ignora linhas mal formadas

    print(f"\nCaptura concluída: {len(dados)} amostras.")
    ser.close()
    return dados


def salvar_csv(dados, nome_arquivo="dados.csv"):
    """Salva os dados em formato CSV."""
    if not dados:
        return
    with open(nome_arquivo, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(["tempo_ms", "setpoint_rpm", "rpm", "pwm"])
        for tempo_ms, setpoint, rpm, pwm in dados:
            writer.writerow([f"{tempo_ms:.3f}", f"{setpoint:.2f}", f"{rpm:.2f}", pwm])
    print(f"Dados salvos em {nome_arquivo}")


def plotar(dados):
    """Gera um gráfico de RPM e PWM ao longo do tempo."""
    if not dados:
        print("Sem dados para plotar.")
        return

    # dados é uma lista de tuplas (tempo_ms, setpoint, rpm, pwm)
    tempo_ms = [d[0] for d in dados]
    setpoints = [d[1] for d in dados]
    rpm = [d[2] for d in dados]
    pwm = [d[3] for d in dados]

    # Evita erro do backend Agg com muitos pontos
    max_points = 200000
    step = max(1, len(tempo_ms) // max_points)
    if step > 1:
        tempo_ms = tempo_ms[::step]
        rpm = rpm[::step]
        pwm = pwm[::step]

    plt.rcParams["agg.path.chunksize"] = 10000

    titulo = "Resposta do Motor - Malha Fechada"

    fig, (ax1, ax2) = plt.subplots(2, 1, sharex=True, figsize=(12, 6))

    ax1.plot(tempo_ms, rpm, 'b-', linewidth=0.5, label="RPM Medido")
    # Plota a linha de setpoint. Pega o primeiro valor, já que deve ser constante.
    ax1.axhline(setpoints[0], color='g', linestyle='--', linewidth=1.0, label=f"Setpoint ({setpoints[0]} RPM)")
    ax1.legend(loc="best")
    ax1.set_ylabel("RPM")
    ax1.set_title(titulo)
    ax1.grid(True, alpha=0.3)

    ax2.plot(tempo_ms, pwm, 'r-', linewidth=0.5)
    ax2.set_ylabel("PWM")
    ax2.set_xlabel("Tempo (ms)")
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig("dados.png", dpi=150)
    print("Gráfico salvo em dados.png")
    plt.show()


def main():
    # Argumentos da linha de comandos
    porta = None
    target_rpm = 200.0  # RPM padrão
    duration_s = 2.0    # Duração padrão em segundos

    args = sys.argv[1:]
    i = 0
    while i < len(args):
        if args[i] == '--rpm' and i + 1 < len(args):
            target_rpm = float(args[i+1])
            i += 2
        elif args[i] == '--dur' and i + 1 < len(args):
            duration_s = float(args[i+1])
            i += 2
        elif not args[i].startswith("-"):
            porta = args[i]
            i += 1
        else:
            print(f"Argumento inválido ou incompleto: {args[i]}")
            sys.exit(1)

    if porta is None:
        porta = encontrar_porta()
        if porta is None:
            print("Nenhuma porta serial do ESP32 foi encontrada. Especifique manualmente:")
            print(f"  python {sys.argv[0]} COM5")
            sys.exit(1)
        print(f"Porta detectada automaticamente: {porta}")

    dados = executar_teste(porta, target_rpm, duration_s)

    if dados:
        salvar_csv(dados)
        plotar(dados)
    else:
        print("Nenhum dado capturado.")


if __name__ == "__main__":
    main()
