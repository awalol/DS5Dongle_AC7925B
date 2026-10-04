#!/usr/bin/env python3
"""生成 ds5dongle 专用控制器库，不修改 SDK 共享静态库。"""

import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import tempfile


# 这些库成员决定内部消息队列的接口约定，以及控制器保留缓冲区和重试的机制。
# 升级 SDK 时，必须重新核对这些约定和机制。
VERIFIED_MEMBERS = (
    ("controller", "hci_controller.c.o",
     "4f38986acfc3293d53c0a87be8eb7bc39ec92300c44fad793d6889ba2865e882"),
    ("controller", "lmp_rxtx_v2.c.o",
     "30a081b21292d833171571d55ec5cbe1a2b893083166ebbc92b110f3d2fd9f22"),
    ("controller", "inquiry_scan.c.o",
     "b51568c19ecc7471cae8a425ed7c65e48c94e22a72dd69b59a16a89823579841"),
    ("controller", "page_scan.c.o",
     "b55c7afc91a6a5750ca0788aed537c04152dd2e827f6786910a3d9d251ef7f79"),
    ("stack", "btstack_task.c.o",
     "19005daf9dcd0467a86b22199d14f5e077a1fb90e7b5be58cf5d4d08e0f733d0"),
)

SCAN_DEBUG_CALLS = {
    "inquiry_scan.c.o": {
        "  %7 = tail call i32 @putchar(i32 73) #8, !dbg !630\n":
            "  %7 = add i32 0, 0\n",
        "  %12 = tail call i32 @dlog_putchar(i8 signext 73) #7, !dbg !638\n":
            "  %12 = add i32 0, 0\n",
    },
    "page_scan.c.o": {
        "  %15 = tail call i32 @putchar(i32 80) #7, !dbg !416\n":
            "  %15 = add i32 0, 0\n",
        "  %20 = tail call i32 @dlog_putchar(i8 signext 80) #8, !dbg !424\n":
            "  %20 = add i32 0, 0\n",
    },
}


def prepare(args):
    originals = {}
    for library, member, expected in VERIFIED_MEMBERS:
        data = subprocess.check_output([args.ar, "p", getattr(args, library), member])
        if hashlib.sha256(data).hexdigest() != expected:
            raise SystemExit(
                f"Unverified Bluetooth ABI: {member} changed. "
                "Re-audit ACL queue, retry, and scan-debug semantics before "
                "updating its hash."
            )
        if member == "hci_controller.c.o" or member in SCAN_DEBUG_CALLS:
            originals[member] = data

    output = Path(args.output).resolve()
    if output in (Path(args.controller).resolve(), Path(args.stack).resolve()):
        raise SystemExit("Output must not overwrite a shared SDK archive")
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".ds5-hci-", dir=output.parent) as tmp:
        tmp = Path(tmp)
        cc = [args.cc, "-target", "pi32v2", "-mcpu=r3", "-mfprev1"]
        patched_members = []

        member = tmp / "hci_controller.c.o"
        member.write_bytes(originals[member.name])
        ir = tmp / "hci_controller.ll"
        subprocess.run(cc + ["-S", "-emit-llvm", "-x", "ir", str(member),
                             "-o", str(ir)], check=True)
        text = ir.read_text()
        symbol = "@hci_send_acl_data("
        if text.count(symbol) != 1 or "define i32 " + symbol not in text:
            raise SystemExit("Unexpected hci_send_acl_data definition/references")
        ir.write_text(text.replace(symbol, "@ds5_sdk_hci_send_acl_data("))
        subprocess.run(cc + ["-c", "-emit-llvm", "-flto", "-x", "ir",
                             "-fprefer-gnu-section", "-mllvm",
                             "-pi32v2-large-program=true", str(ir),
                             "-o", str(member)], check=True)
        patched_members.append(member)

        if args.scan_debug == "0":
            for member_name, debug_calls in SCAN_DEBUG_CALLS.items():
                member = tmp / member_name
                member.write_bytes(originals[member_name])
                ir = tmp / member_name.replace(".c.o", ".ll")
                subprocess.run(cc + ["-S", "-emit-llvm", "-x", "ir", str(member),
                                     "-o", str(ir)], check=True)
                text = ir.read_text()
                for debug_call, replacement in debug_calls.items():
                    if text.count(debug_call) != 1:
                        raise SystemExit(
                            f"Unexpected scan debug call in {member_name}: "
                            f"{debug_call.strip()}"
                        )
                    text = text.replace(debug_call, replacement)
                ir.write_text(text)
                subprocess.run(cc + ["-c", "-emit-llvm", "-flto", "-x", "ir",
                                     "-fprefer-gnu-section", "-mllvm",
                                     "-pi32v2-large-program=true", str(ir),
                                     "-o", str(member)], check=True)
                patched_members.append(member)

        archive = tmp / "btctrler_ds5.a"
        shutil.copyfile(args.controller, archive)
        subprocess.run([args.ar, "rcs", str(archive),
                        *(str(member) for member in patched_members)], check=True)
        archive.replace(output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("cc", "ar", "controller", "stack", "output"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--scan-debug", choices=("0", "1"), required=True,
                        help="1 keeps the controller's IP scan characters")
    prepare(parser.parse_args())
