import os
import shutil
import sys
from pathlib import Path
import typing as t

__version__: str = "0.1.0"

HEADER_FILES: list = [
    "wifi.h"
]


def log_method_call(func: t.Callable, **kwargs) -> None:
    log: str = f"-{func.__name__}"

    for key, val in kwargs.items():
        log += f" {val}"
    
    print(f"{log}")
    func(**kwargs)

def main() -> None:
    print(f"idf-install.py tool {__version__}")

    abspath: t.AnyStr = os.path.abspath(sys.argv[0])
    script_path_untrue: t.AnyStr = os.path.dirname(abspath)

    ck_path: Path = Path(f"{script_path_untrue.replace('install_scripts', '')}")
    
    unos_path: str = input("Where is your ESP-IDF Installation? ")
    print("-----------------------------------------------------\n")

    os_path: Path = Path(unos_path)
    if (os_path.exists() == False):
        print("ERROR: Path does not exist...")
        exit(-1)

    elif (os_path.is_file()):
        print("ERROR: Path is file...")
        exit(-1)

    log_method_call(os.chdir, path=f"{os_path}/components")

    log_method_call(os.mkdir, path="ck_wifi_helper")
    log_method_call(func=os.chdir, path="ck_wifi_helper")

    log_method_call(os.mkdir, path="include")
    log_method_call(os.chdir, path="include")

    log_method_call(os.mkdir, path="ck_wifi_helper")

    for header in HEADER_FILES:
        header_path: Path = Path(f'{ck_path}/include/ck_wifi_helper/{header}')
        if (header_path.exists() != True):
            print(f"ERROR: {header_path} does not exist!")
            exit(-1)

        log_method_call(shutil.copyfile, src=header_path, dst=str(Path("./ck_wifi_helper/wifi.h")))

if __name__ == "__main__":
    main()