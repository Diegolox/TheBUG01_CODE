"""Terminal de inclinación para TheBUG01 en Windows."""

import argparse
import msvcrt

import serial
from serial.tools import list_ports


def run_terminal(connection):
    print("Teclas: c capturar Roll, + / - ajustar 0,1°, 0 restaurar, x desarmar, q salir.")
    while True:
        while msvcrt.kbhit():
            key = msvcrt.getwch()
            if key in ("\x00", "\xe0"):
                msvcrt.getwch()  # Descartar la segunda parte de una tecla especial.
                continue
            if key in ("q", "Q", "\x03"):
                return
            command = key.lower()
            if command in ("+", "-", "0", "c", "x"):
                connection.write(command.encode("ascii"))

        telemetry = connection.read(connection.in_waiting or 1)
        if telemetry:
            print(telemetry.decode("ascii", errors="replace"), end="", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", nargs="?", help="Puerto COM Bluetooth saliente o USB.")
    parser.add_argument("--list", action="store_true", help="Mostrar los puertos disponibles.")
    args = parser.parse_args()

    if args.list:
        ports = sorted(list_ports.comports())
        for port in ports:
            print(f"{port.device}: {port.description}")
        if not ports:
            print("No se encontraron puertos serie.")
        return 0

    if args.port is None:
        parser.error("indica un puerto COM o usa --list")

    try:
        with serial.Serial(args.port, 115200, timeout=0.05, write_timeout=0.5) as connection:
            print(f"Conectado a {connection.port}")
            run_terminal(connection)
    except serial.SerialException as exc:
        parser.exit(1, f"Error en {args.port}: {exc}\n")
    except KeyboardInterrupt:
        pass

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
