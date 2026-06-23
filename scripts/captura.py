"""
captura.py — Captura de dados do ESP32 via Serial (streaming binário)

Fluxo:
  1. Detecta automaticamente a porta serial do ESP32 (ou usa porta manual)
  2. Envia comando com parâmetros para iniciar o teste
  3. Recebe pacotes binários de 8 bytes em tempo real
  4. Quando o teste termina (END_MARKER), salva CSV e mostra gráfico

Formato do pacote (8 bytes, little-endian):
    [0-3] uint32  time_us   — tempo desde o início (µs)
    [4-5] int16   rpm_div2  — RPM / 2 (resolução 2 RPM, com sinal)
    [6-7] int16   pwm       — duty cycle

Uso:
  python captura.py                                 # frequência, auto-detecção
  python captura.py --mode step                     # degrau, auto-detecção
  python captura.py COM5                            # porta específica
  python captura.py --mode step --dur 5 --pwm 2048  # degrau, PWM 2048, 5s
  python captura.py --mode freq --freq 2.0 --amp 2048  # freq 2Hz, amp 2048
  python captura.py --mode closed --setpoint 100 --dur 10  # malha fechada
  python captura.py --avg 10                          # janela média móvel = 10

Parâmetros:
  --mode freq|step|closed|mf Tipo de teste (padrão: freq)
  --dur N              Duração em segundos (padrão: 30)
  --pwm N              PWM para teste de degrau (padrão: 4095)
  --offset N           Offset PWM para senoide (padrão: 0)
  --amp N              Amplitude PWM para senoide (padrão: 4095)
  --freq N             Frequência em Hz para senoide (padrão: 1.0)
  --setpoint N         Referencia de RPM para malha fechada (padrão: 100)
  --avg N              Tamanho da janela de média móvel do encoder (1-32)
"""

import serial
import serial.tools.list_ports
import struct
import sys
import time
import csv
import matplotlib.pyplot as plt

# --- Constantes (devem coincidir com Constants.h) ---
END_MARKER = 0x7FFF
BAUD_RATE = 1500000
PACKET_SIZE = 8  # 4 + 2 + 2 bytes
SAMPLE_PERIOD_US = 60
SAMPLE_RATE_HZ = int(1_000_000 / SAMPLE_PERIOD_US)

# --- Configuração ---
DEFAULT_DURATION_S = 30
DEFAULT_MODE = "freq"  # "freq" (senoide) ou "step" (degrau)

# Parâmetros padrão (iguais a Constants.h)
DEFAULT_STEP_PWM = 4095
DEFAULT_FREQ_OFFSET = 0
DEFAULT_FREQ_AMP = 4095
DEFAULT_FREQ_HZ = 1.0
DEFAULT_CLOSED_SETPOINT_RPM = 100.0


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


def ler_pacote(ser):
    """
    Lê um pacote fixo de 7 bytes do Serial.
    Retorna (time_us, rpm, pwm) ou None se timeout/fim de teste.
    """
    data = ser.read(PACKET_SIZE)
    if len(data) < PACKET_SIZE:
        return None

    # [0-3] uint32 time_us, [4-5] int16 rpm_div2, [6-7] int16 pwm
    time_us, rpm_div2, pwm = struct.unpack('<Ihh', data)

    # Verifica se é marcador de fim de teste
    if rpm_div2 == END_MARKER:
        return "END"

    rpm = rpm_div2 * 2.0
    return (time_us, rpm, pwm)


def executar_teste(porta, duracao_s, mode="freq", params=None, avg_window=None):
    """Executa o teste e retorna os dados capturados."""
    if params is None:
        params = {}

    print(f"Conectando a {porta} a {BAUD_RATE} baud...")
    ser = serial.Serial(porta, BAUD_RATE, timeout=2)
    time.sleep(0.5)  # Aguarda estabilizar

    # Limpa buffer de entrada
    ser.reset_input_buffer()

    # Envia configuração de janela de média móvel (se especificada)
    if avg_window is not None:
        cmd_avg = f"W:{avg_window}\n"
        print(f"Configurando janela de média móvel: {avg_window}")
        ser.write(cmd_avg.encode())
        time.sleep(0.1)

    # Monta e envia comando com parâmetros
    if mode == "step":
        pwm = params.get("pwm", DEFAULT_STEP_PWM)
        cmd = f"S:{pwm},{int(duracao_s)}\n"
        print(f"Enviando comando de degrau: PWM={pwm}, duração={duracao_s}s")
        ser.write(cmd.encode())
    elif mode in ("closed", "mf"):
        setpoint = params.get("setpoint", DEFAULT_CLOSED_SETPOINT_RPM)
        cmd = f"M:{setpoint:.2f},{int(duracao_s)}\n"
        print(f"Enviando comando de malha fechada: setpoint={setpoint} RPM, duracao={duracao_s}s")
        ser.write(cmd.encode())
    else:
        offset = params.get("offset", DEFAULT_FREQ_OFFSET)
        amp = params.get("amp", DEFAULT_FREQ_AMP)
        freq = params.get("freq", DEFAULT_FREQ_HZ)
        cmd = f"F:{offset},{amp},{freq:.2f},{int(duracao_s)}\n"
        print(f"Enviando comando de frequência: offset={offset}, amp={amp}, freq={freq}Hz, duração={duracao_s}s")
        ser.write(cmd.encode())

    dados = []
    inicio = time.time()
    amostras_esperadas = int(duracao_s * SAMPLE_RATE_HZ)

    print(f"Capturando dados por {duracao_s} segundos (~{amostras_esperadas} amostras)...")

    while True:
        resultado = ler_pacote(ser)

        if resultado is None:
            print("Timeout na leitura.")
            break

        if resultado == "END":
            print("Marcador de fim de teste recebido.")
            break

        dados.append(resultado)

        # Progresso a cada 1000 amostras
        if len(dados) % 1000 == 0:
            elapsed = time.time() - inicio
            print(f"  {len(dados)} amostras capturadas ({elapsed:.1f}s)")

        # Timeout de segurança
        if time.time() - inicio > duracao_s + 5:
            print("Timeout de segurança atingido.")
            break

    ser.close()

    tempo_total = time.time() - inicio
    print(f"Captura concluída: {len(dados)} amostras em {tempo_total:.1f}s")
    if len(dados) > 1:
        taxa = len(dados) / (dados[-1][0] - dados[0][0]) * 1e6
        print(f"Taxa real de amostragem: {taxa:.0f} Hz")

    return dados


def salvar_csv(dados, nome_arquivo="dados.csv", mode="freq", setpoint=None):
    """Salva os dados em formato CSV."""
    with open(nome_arquivo, 'w', newline='') as f:
        writer = csv.writer(f)
        if mode in ("closed", "mf") and setpoint is not None:
            writer.writerow(["tempo_ms", "setpoint_rpm", "rpm", "pwm"])
            for time_us, rpm, pwm in dados:
                writer.writerow([f"{time_us / 1000.0:.3f}", f"{setpoint:.2f}", f"{rpm:.2f}", pwm])
        else:
            writer.writerow(["tempo_ms", "rpm", "pwm"])
            for time_us, rpm, pwm in dados:
                writer.writerow([f"{time_us / 1000.0:.3f}", f"{rpm:.2f}", pwm])
    print(f"Dados salvos em {nome_arquivo}")


def plotar(dados, mode="freq", setpoint=None):
    """Gera gráfico de RPM e PWM ao longo do tempo."""
    if not dados:
        print("Sem dados para plotar.")
        return

    tempo_ms = [d[0] / 1000.0 for d in dados]
    rpm = [d[1] for d in dados]
    pwm = [d[2] for d in dados]

    # Evita erro do backend Agg com muitos pontos
    max_points = 200000
    step = max(1, len(tempo_ms) // max_points)
    if step > 1:
        tempo_ms = tempo_ms[::step]
        rpm = rpm[::step]
        pwm = pwm[::step]

    plt.rcParams["agg.path.chunksize"] = 10000

    if mode == "step":
        titulo = "Resposta do Motor - Teste de Degrau"
    elif mode in ("closed", "mf"):
        titulo = "Resposta do Motor - Malha Fechada"
    else:
        titulo = "Resposta do Motor - Teste de Frequencia"

    fig, (ax1, ax2) = plt.subplots(2, 1, sharex=True, figsize=(12, 6))

    ax1.plot(tempo_ms, rpm, 'b-', linewidth=0.5)
    if mode in ("closed", "mf") and setpoint is not None:
        ax1.axhline(setpoint, color='g', linestyle='--', linewidth=1.0, label="Setpoint")
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
    duracao = DEFAULT_DURATION_S
    mode = DEFAULT_MODE
    params = {}
    avg_window = None

    args = sys.argv[1:]
    i = 0
    while i < len(args):
        if args[i] in ("--dur", "-dur") and i + 1 < len(args):
            duracao = float(args[i + 1])
            i += 2
        elif args[i] in ("--mode", "-mode") and i + 1 < len(args):
            mode = args[i + 1].lower()
            if mode not in ("freq", "step", "closed", "mf"):
                print(f"Modo invalido: {mode}. Use 'freq', 'step', 'closed' ou 'mf'.")
                sys.exit(1)
            i += 2
        elif args[i] in ("--pwm", "-pwm") and i + 1 < len(args):
            params["pwm"] = int(args[i + 1])
            i += 2
        elif args[i] in ("--offset", "-offset") and i + 1 < len(args):
            params["offset"] = int(args[i + 1])
            i += 2
        elif args[i] in ("--amp", "-amp") and i + 1 < len(args):
            params["amp"] = int(args[i + 1])
            i += 2
        elif args[i] in ("--freq", "-freq") and i + 1 < len(args):
            params["freq"] = float(args[i + 1])
            i += 2
        elif args[i] in ("--setpoint", "-setpoint") and i + 1 < len(args):
            params["setpoint"] = float(args[i + 1])
            i += 2
        elif args[i] in ("--avg", "-avg") and i + 1 < len(args):
            avg_window = int(args[i + 1])
            i += 2
        elif not args[i].startswith("-"):
            porta = args[i]
            i += 1
        else:
            print(f"Argumento invalido ou incompleto: {args[i]}")
            sys.exit(1)

    if porta is None:
        porta = encontrar_porta()
        if porta is None:
            print("Nenhuma porta serial encontrada. Especifique manualmente:")
            print("  python captura.py COM5")
            sys.exit(1)
        print(f"Porta detectada automaticamente: {porta}")

    dados = executar_teste(porta, duracao, mode, params, avg_window)

    if dados:
        setpoint = params.get("setpoint", DEFAULT_CLOSED_SETPOINT_RPM) if mode in ("closed", "mf") else None
        salvar_csv(dados, mode=mode, setpoint=setpoint)
        plotar(dados, mode, setpoint)
    else:
        print("Nenhum dado capturado.")


if __name__ == "__main__":
    main()
