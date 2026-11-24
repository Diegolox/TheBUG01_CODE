"""Terminal Bluetooth para TheBUG01 en Windows."""

import argparse
import msvcrt
import os
import re
from concurrent.futures import Future
from threading import Thread
from typing import NamedTuple
import winreg

import serial
from serial.tools import list_ports

DEVICE_REGISTRY = r"SYSTEM\CurrentControlSet\Services\BTHPORT\Parameters\Devices"
REMOTE_ADDRESS = re.compile(r"&([0-9a-f]{12})_[0-9a-f]+$", re.IGNORECASE)
COMMANDS = frozenset("ac+-0xpPdD")


class BluetoothPort(NamedTuple):
    device: str
    name: str


def device_name(address):
    try:
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, f"{DEVICE_REGISTRY}\\{address}") as key:
            name, _ = winreg.QueryValueEx(key, "Name")
    except OSError:
        return ""
    return name.rstrip(b"\0").decode("utf-8", errors="replace") if isinstance(name, bytes) else str(name)


def bluetooth_ports(ports):
    """Identificar puertos Bluetooth y omitir los entrantes que Windows marca con dirección cero."""
    candidates = []
    for port in ports:
        if not port.hwid.upper().startswith("BTHENUM\\") and "bluetooth" not in port.description.casefold():
            continue
        match = REMOTE_ADDRESS.search(port.hwid)
        address = match.group(1) if match else ""
        if address == "000000000000":
            continue
        candidates.append(BluetoothPort(port.device, device_name(address) if address else ""))
    return sorted(candidates, key=lambda port: (not port.name.lower().startswith("thebug01"), port.device))


def run_terminal(connection):
    print("Puerto abierto. Los comandos se envían directamente al robot.")
    print("Teclas  a armar · x parar · c capturar · +/- ajustar · 0 restaurar · q desconectar")
    print("Ganancias PD  p/P subir/bajar Kp · d/D subir/bajar Kd")
    pending = b""

    while True:
        while msvcrt.kbhit():
            key = msvcrt.getwch()
            if key not in "pPdD":
                key = key.lower()
            if key in ("\x00", "\xe0"):
                msvcrt.getwch()  # Segunda parte de una tecla especial de Windows.
                continue
            if key in ("q", "\x1b", "\x03"):
                return
            if key in COMMANDS:
                connection.write(key.encode("ascii"))

        data = connection.read(min(connection.in_waiting or 1, 4096))
        if data:
            *lines, pending = (pending + data).split(b"\n")
            for line in lines:
                print(line.rstrip(b"\r").decode("ascii", errors="replace"), flush=True)
            pending = pending[-4096:]


def open_port(port, connect_timeout=8.0):
    result = Future()

    def open_serial():
        try:
            connection = serial.Serial(port, 115200, timeout=0.05, write_timeout=0.5)
        except OSError as exc:
            result.set_exception(exc)
        else:
            result.set_result(connection)

    Thread(target=open_serial, daemon=True).start()
    try:
        return result.result(timeout=connect_timeout)
    except (TimeoutError, KeyboardInterrupt):
        def close_late_connection(finished):
            try:
                finished.result().close()
            except OSError:
                pass

        result.add_done_callback(close_late_connection)
        raise


def connect(port):
    print(f"\nAbriendo {port}…", flush=True)
    try:
        with open_port(port) as connection:
            run_terminal(connection)
    except TimeoutError:
        print(f"{port} · El puerto no respondió a tiempo. Comprueba que el robot esté encendido y emparejado.")
        return 1
    except OSError as exc:
        print(f"{port} · {exc}")
        return 1
    return 0


def connection_menu():
    while True:
        ports = bluetooth_ports(list_ports.comports())
        print("\nTheBUG01 / Terminal Bluetooth")
        for index, port in enumerate(ports, start=1):
            print(f"  {index}. {port.device}  {port.name or 'Bluetooth'}")
        if not ports:
            print("  No hay puertos Bluetooth disponibles.")

        named_robot = [port for port in ports if port.name.lower().startswith("thebug01")]
        default_port = named_robot[0].device if len(named_robot) == 1 else None
        print("  e  Emparejar en Windows   r  Actualizar   q  Salir")
        prompt = f"Elige puerto [{default_port}]: " if default_port else "Elige puerto: "
        choice = input(prompt).strip().lower()
        if choice == "q":
            return 0
        if choice == "e":
            try:
                os.startfile("ms-settings:bluetooth")
                input("Empareja TheBUG01-PD en Windows y vuelve aquí. Pulsa Intro para actualizar.")
            except OSError as exc:
                print(f"No se pudo abrir Configuración de Bluetooth. {exc}")
        elif not choice and default_port:
            connect(default_port)
        elif choice.isdecimal() and 1 <= int(choice) <= len(ports):
            connect(ports[int(choice) - 1].device)
        elif choice != "r":
            print("Introduce el número de un puerto, e, r o q.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", nargs="?", help="Conectar directamente a un COM Bluetooth o USB.")
    parser.add_argument("--list", action="store_true", help="Mostrar los puertos disponibles.")
    args = parser.parse_args()

    if args.list:
        ports = sorted(list_ports.comports())
        names = {port.device: port.name for port in bluetooth_ports(ports)}
        for port in ports:
            label = names.get(port.device) or port.description
            print(f"{port.device}  {label}")
        if not ports:
            print("No se encontraron puertos serie.")
        return 0

    try:
        return connect(args.port) if args.port else connection_menu()
    except (EOFError, KeyboardInterrupt):
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
