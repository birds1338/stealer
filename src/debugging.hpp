#pragma once
#include "util.hpp"
#include "xor.hpp"
#include <Psapi.h>
#include <TlHelp32.h>
#include <filesystem>
#include <handleapi.h>

inline bool is_virtualbox() {
  HANDLE device = CreateFileA(xor("\\\\.\\VBoxMiniRdrDN"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (device != INVALID_HANDLE_VALUE) {
    CloseHandle(device);
    return true;
  }

  std::filesystem::path paths[] = {
      xor("C:\\Windows\\System32\\vboxhook.dll"),
      xor("C:\\Windows\\System32\\vboxmrxnp.dll"),
  };

  for (const auto &path : paths) {
    if (std::filesystem::exists(path))
      return true;
  }

  return false;
}

inline bool is_vmware() { return RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\VMware, Inc.", NULL) == ERROR_SUCCESS; }

inline bool is_hyperv() {
  auto manufacturer = read_registry_key<std::string>(HKEY_LOCAL_MACHINE, "SYSTEM\\HardwareConfig\\Current", "SystemManufacturer");
  if (manufacturer.find("Microsoft") != std::string::npos)
    return true;

  auto bios = read_registry_key<std::string>(HKEY_LOCAL_MACHINE, "SYSTEM\\HardwareConfig\\Current", "BIOSVendor");
  if (bios.find("Microsoft") != std::string::npos)
    return true;

  auto family = read_registry_key<std::string>(HKEY_LOCAL_MACHINE, "SYSTEM\\HardwareConfig\\Current", "SystemFamily");
  if (family.find("Virtual Machine") != std::string::npos)
    return true;

  auto product = read_registry_key<std::string>(HKEY_LOCAL_MACHINE, "SYSTEM\\HardwareConfig\\Current", "ProductName");
  if (product.find("Virtual Machine") != std::string::npos)
    return true;

  return false;
}

inline bool is_debugged() {
  const char *processes[] = {
      xor("vmtoolsd.exe"), xor("vmwaretray.exe"), xor("vmwareuser.exe"), xor("fakenet.exe"), xor("dumpcap.exe"),       xor("httpdebuggerui.exe"), xor("wireshark.exe"), xor("fiddler.exe"),      xor("vboxservice.exe"),
      xor("df5serv.exe"),  xor("vboxtray.exe"),   xor("ida64.exe"),      xor("ollydbg.exe"), xor("pestudio.exe"),      xor("vgauthservice.exe"),  xor("vmacthlp.exe"),  xor("x96dbg.exe"),       xor("x32dbg.exe"),
      xor("prl_cc.exe"),   xor("prl_tools.exe"),  xor("xenservice.exe"), xor("qemu-ga.exe"), xor("joeboxcontrol.exe"), xor("ksdumperclient.exe"), xor("ksdumper.exe"),  xor("joeboxserver.exe"),
  };

  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  PROCESSENTRY32 entry;
  entry.dwSize = sizeof(entry);

  if (Process32First(snapshot, &entry)) {
    do {
      for (const auto &process : processes) {
        if (strcmp(entry.szExeFile, process) == 0) {
          CloseHandle(snapshot);
          return true;
        }
      }
    } while (Process32Next(snapshot, &entry));
  }

  return is_virtualbox() || is_vmware() || is_hyperv();
}