/*
  VitaVault - PSVita Backup Utility
  USB Mass Storage Support (adapted from VitaShell)
  
  Copyright (C) 2015-2018, TheFloW (original VitaShell USB code)
  Copyright (C) 2026 theheroGAC (VitaVault adaptation)
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>
#include <psp2/vshbridge.h>
#include <psp2/appmgr.h>
#include <psp2/usbstorvstor.h>
#include <psp2/mtpif.h>
#include <psp2/shellutil.h>
#include <psp2/types.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <taihen.h>

#include "usb.h"
#include "ui.h"
#include "backup.h"
#include "types.h"


int g_usb_active = 0;
SceUID g_usb_modid = -1;

static int g_usb_modules_loaded = 0;
static SceUID g_kernel_modid = -1;
static SceUID g_patch_modid = -1;
static SceUID g_umass_modid = -1;
static SceUID g_user_modid = -1;

static const char *USB_MODULE_DIR = "ux0:data/VitaVault/module/";

static const char *USB_MODULE_FILES[] = {
    "kernel.skprx",
    "patch.skprx",
    "usbdevice.skprx",
    "user.suprx",
    "umass.skprx",
    NULL
};

static int load_usb_modules(void);

static int start_kernel_module(const char *path, const char *search_name, SceUID *out_modid) {
    int search_unk[2] = {0, 0};

    if (search_name) {
        SceUID existing = _vshKernelSearchModuleByName(search_name, search_unk);
        if (existing >= 0) {
            *out_modid = existing;
            return 0;
        }
    }

    SceUID modid = taiLoadKernelModule(path, 0, NULL);
    if (modid == 0x8002D021) {
        if (search_name)
            modid = _vshKernelSearchModuleByName(search_name, search_unk);
        if (modid < 0)
            return modid;
    } else if (modid < 0) {
        return modid;
    }

    int res = taiStartKernelModule(modid, 0, NULL, 0, NULL, NULL);
    if (res < 0 && res != 0x8002D022) {
        taiStopUnloadKernelModule(modid, 0, NULL, 0, NULL, NULL);
        return res;
    }

    *out_modid = modid;
    return 0;
}


int checkFolderExist(const char *path) {
    SceUID dfd = sceIoDopen(path);
    if (dfd >= 0) {
        sceIoDclose(dfd);
        return 1;
    }
    return 0;
}


static int checkFileExist(const char *path) {
    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
    if (fd >= 0) {
        sceIoClose(fd);
        return 1;
    }
    return 0;
}


static int copyFile(const char *src, const char *dst) {
    SceUID fdsrc = sceIoOpen(src, SCE_O_RDONLY, 0);
    if (fdsrc < 0) return -1;

    SceUID fddst = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fddst < 0) {
        sceIoClose(fdsrc);
        return -1;
    }

    char buf[16384];
    int read;
    while ((read = sceIoRead(fdsrc, buf, sizeof(buf))) > 0) {
        sceIoWrite(fddst, buf, read);
    }

    sceIoClose(fdsrc);
    sceIoClose(fddst);
    return 0;
}

static int install_usb_modules(void) {
    int kernel_exists = checkFileExist("ux0:data/VitaVault/module/kernel.skprx");
    int umass_exists = checkFileExist("ux0:data/VitaVault/module/umass.skprx");
    int vitashell_umass_exists = checkFileExist("ux0:VitaShell/module/umass.skprx");

    
    if (kernel_exists && umass_exists && vitashell_umass_exists)
        return 0;

    sceIoMkdir("ux0:data", 0777);
    sceIoMkdir("ux0:data/VitaVault", 0777);
    sceIoMkdir("ux0:data/VitaVault/module", 0777);
    sceIoMkdir("ux0:VitaShell", 0777);
    sceIoMkdir("ux0:VitaShell/module", 0777);

    int modules_installed = 0;

    for (int i = 0; USB_MODULE_FILES[i]; i++) {
        char src[256];
        char dst[256];
        snprintf(src, sizeof(src), "app0:module/%s", USB_MODULE_FILES[i]);
        snprintf(dst, sizeof(dst), "%s%s", USB_MODULE_DIR, USB_MODULE_FILES[i]);

        if (checkFileExist(dst)) {
            modules_installed++;
            continue;
        }

        if (!checkFileExist(src)) {
            // Try alternative path if app0:module/ doesn't work
            snprintf(src, sizeof(src), "ux0:app/VTBK00001/module/%s", USB_MODULE_FILES[i]);
            if (!checkFileExist(src)) {
                continue;
            }
        }

        if (copyFile(src, dst) < 0)
            return 0x80800002;

        modules_installed++;

        if (strcmp(USB_MODULE_FILES[i], "umass.skprx") == 0) {
            if (!vitashell_umass_exists) {
                if (copyFile(src, "ux0:VitaShell/module/umass.skprx") < 0)
                    return 0x80800002;
            }
        }
    }

    
    if (!checkFileExist("ux0:data/VitaVault/module/kernel.skprx") ||
        !checkFileExist("ux0:data/VitaVault/module/umass.skprx")) {
        return 0x80800001;
    }

    return 0;
}


static int copyPath(const char *src, const char *dst, const char *parent) {
    SceUID fd = sceIoOpen(src, SCE_O_RDONLY, 0);
    if (fd >= 0) {
        
        sceIoClose(fd);
        return copyFile(src, dst);
    }

    
    SceUID dir = sceIoDopen(src);
    if (dir < 0) return -1;

    sceIoMkdir(dst, 0777);

    SceIoDirent ent;
    memset(&ent, 0, sizeof(ent));
    while (sceIoDread(dir, &ent) > 0) {
        if (strcmp(ent.d_name, ".") == 0 || strcmp(ent.d_name, "..") == 0) {
            memset(&ent, 0, sizeof(ent));
            continue;
        }

        char src_path[PATH_MAX_SIZE];
        char dst_path[PATH_MAX_SIZE];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, ent.d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, ent.d_name);

        copyPath(src_path, dst_path, parent);
        memset(&ent, 0, sizeof(ent));
    }
    sceIoDclose(dir);
    return 0;
}


static int shellUserRedirectUx0(const char *src, const char *dst) {
    uint32_t buf[3];
    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;
    return _vshIoMount(0x800, src, 0, buf);
}

SceUID startUsb(const char *usbDevicePath, const char *imgFilePath, int type) {
    SceUID modid = -1;
    int res;

    sceAppMgrDestroyOtherApp();

    res = taiLoadStartKernelModule(usbDevicePath, 0, NULL, 0);
    if (res == 0x8002D021) {
        int search_unk[2] = {0, 0};
        res = _vshKernelSearchModuleByName("SceUsbDeviceController", search_unk);
    }
    if (res < 0)
        goto ERROR_LOAD_MODULE;

    modid = res;

    
    res = sceMtpIfStopDriver(1);
    if (res < 0 && res != 0x8054360C)
        goto ERROR_STOP_DRIVER;

    
    res = sceUsbstorVStorSetDeviceInfo("\"PS Vita\" MC", "1.00");
    if (res < 0)
        goto ERROR_USBSTOR_VSTOR;

    
    res = sceUsbstorVStorSetImgFilePath(imgFilePath);
    if (res < 0)
        goto ERROR_USBSTOR_VSTOR;

    
    res = sceUsbstorVStorStart(type);
    if (res < 0)
        goto ERROR_USBSTOR_VSTOR;

    return modid;

ERROR_USBSTOR_VSTOR:
    sceMtpIfStartDriver(1);

ERROR_STOP_DRIVER:
    if (modid > 0) taiStopUnloadKernelModule(modid, 0, NULL, 0, NULL, NULL);

ERROR_LOAD_MODULE:
    return res;
}

int stopUsb(SceUID modid) {
    int res;

    
    res = sceUsbstorVStorStop();
    if (res < 0)
        return res;

    
    res = sceMtpIfStartDriver(1);
    if (res < 0)
        return res;

    
    res = taiStopUnloadKernelModule(modid, 0, NULL, 0, NULL, NULL);
    if (res < 0)
        return res;

    return 0;
}

void usb_init(void) {
    sceShellUtilInitEvents(0);
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_USB_CONNECTION);
}

static int storage_candidate_seen(const char *list[], int n, const char *path) {
    for (int i = 0; i < n; i++) {
        if (strcmp(list[i], path) == 0)
            return 1;
    }
    return 0;
}

static void add_storage_candidate(const char *list[], int *n, int max, const char *path) {
    if (*n >= max || storage_candidate_seen(list, *n, path))
        return;
    list[(*n)++] = path;
}

static int build_storage_candidates(const char *candidates[], int max) {
    int n = 0;

    if (checkFolderExist("ux0:data/sd2vita.txt"))
        add_storage_candidate(candidates, &n, max, "sdstor0:gcd-lp-ign-entire");

    if (checkFolderExist("xmc0:"))
        add_storage_candidate(candidates, &n, max, "sdstor0:xmc-lp-ign-userext");

    if (checkFolderExist("imc0:"))
        add_storage_candidate(candidates, &n, max, "sdstor0:int-lp-ign-userext");

    if (checkFolderExist("uma0:")) {
        add_storage_candidate(candidates, &n, max, "sdstor0:uma-lp-act-entire");
        add_storage_candidate(candidates, &n, max, "sdstor0:uma-pp-act-a");
    }

    if (checkFolderExist("grw0:"))
        add_storage_candidate(candidates, &n, max, "sdstor0:gcd-lp-ign-entire");

    add_storage_candidate(candidates, &n, max, "sdstor0:xmc-lp-ign-userext");
    add_storage_candidate(candidates, &n, max, "sdstor0:int-lp-ign-userext");
    add_storage_candidate(candidates, &n, max, "sdstor0:gcd-lp-ign-entire");
    add_storage_candidate(candidates, &n, max, "sdstor0:uma-lp-act-entire");

    return n;
}

static int load_usb_modules(void) {
    char path[256];
    int res;

    if (g_usb_modules_loaded)
        return 0;

    res = install_usb_modules();
    if (res < 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "USB module install: 0x%08X", res);
        ui_set_notification(msg);
        return res;
    }

    
    snprintf(path, sizeof(path), "%skernel.skprx", USB_MODULE_DIR);
    res = start_kernel_module(path, "VitaShellKernel2", &g_kernel_modid);
    if (res < 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Kernel module: 0x%08X", res);
        ui_set_notification(msg);
        return res;
    }

    
    snprintf(path, sizeof(path), "%sumass.skprx", USB_MODULE_DIR);
    res = start_kernel_module(path, "SceUsbMass", &g_umass_modid);
    if (res < 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "USB mass driver: 0x%08X", res);
        ui_set_notification(msg);
        return res;
    }

    
    snprintf(path, sizeof(path), "%spatch.skprx", USB_MODULE_DIR);
    res = start_kernel_module(path, "VitaShellPatch", &g_patch_modid);
    if (res < 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Patch module: 0x%08X", res);
        ui_set_notification(msg);
        return res;
    }

    
    snprintf(path, sizeof(path), "%suser.suprx", USB_MODULE_DIR);
    g_user_modid = sceKernelLoadStartModule(path, 0, NULL, 0, NULL, NULL);
    if (g_user_modid < 0 && g_user_modid != 0x8002D021) {
        char msg[64];
        snprintf(msg, sizeof(msg), "User module: 0x%08X", g_user_modid);
        ui_set_notification(msg);
        return g_user_modid;
    }

    g_usb_modules_loaded = 1;
    return 0;
}

int usb_start_mass_storage_with_device(const char *device_path) {
    char modulePath[256];
    int res;

    if (g_usb_active)
        return 0;

    res = load_usb_modules();
    if (res < 0)
        return res;

    snprintf(modulePath, sizeof(modulePath), "%susbdevice.skprx", USB_MODULE_DIR);

    SceUID modid = startUsb(modulePath, device_path, USBSTOR_TYPE_FAT);
    if (modid >= 0) {
        g_usb_modid = modid;
        g_usb_active = 1;
        return 0;
    }

    return modid;
}

int usb_start_mass_storage(void) {
    const char *candidates[8];
    int nc;
    int last_err = 0x80800003;

    nc = build_storage_candidates(candidates, 8);
    if (nc == 0)
        return 0x80800003;

    for (int i = 0; i < nc; i++) {
        int res = usb_start_mass_storage_with_device(candidates[i]);
        if (res == 0) {
            return 0;
        }
        last_err = res;
    }

    return last_err;
}

int usb_stop_mass_storage() {
    if (!g_usb_active) {
        return 0; 
    }

    int res = stopUsb(g_usb_modid);
    if (res < 0)
        return res;

    g_usb_modid = -1;
    g_usb_active = 0;
    return 0;
}

// Mount gamecard to ux0: (from VitaShell)
int mountGamecardUx0() {
    sceAppMgrDestroyOtherApp();

    copyPath("ux0:app/VITAVAULT", "ur0:temp/app", NULL);
    copyPath("ux0:appmeta/VITAVAULT", "ur0:temp/appmeta", NULL);
    copyPath("ux0:license/app/VITAVAULT", "ur0:temp/license", NULL);

    shellUserRedirectUx0("sdstor0:gcd-lp-ign-entire", "sdstor0:gcd-lp-ign-entire");

    remount(0x800);

    sceIoMkdir("ux0:app", 0006);
    sceIoMkdir("ux0:appmeta", 0006);
    sceIoMkdir("ux0:license", 0006);
    sceIoMkdir("ux0:license/app", 0006);
    sceIoMkdir("ux0:app/VITAVAULT", 0006);
    sceIoMkdir("ux0:appmeta/VITAVAULT", 0006);
    sceIoMkdir("ux0:license/app/VITAVAULT", 0006);

    sceIoMkdir("ux0:data", 0777);
    sceIoMkdir("ux0:temp", 0006);

    copyPath("ur0:temp/app", "ux0:app/VITAVAULT", NULL);
    copyPath("ur0:temp/appmeta", "ux0:appmeta/VITAVAULT", NULL);
    copyPath("ur0:temp/license", "ux0:license/app/VITAVAULT", NULL);

    return 0;
}

// Unmount gamecard from ux0: (from VitaShell)
int umountGamecardUx0() {
    sceAppMgrDestroyOtherApp();

    if (checkFileExist("sdstor0:xmc-lp-ign-userext"))
        shellUserRedirectUx0("sdstor0:xmc-lp-ign-userext", "sdstor0:xmc-lp-ign-userext");
    else
        shellUserRedirectUx0("sdstor0:int-lp-ign-userext", "sdstor0:int-lp-ign-userext");

    remount(0x800);

    return 0;
}

// Mount USB (uma0:) to ux0: (from VitaShell)
int mountUsbUx0() {
    sceAppMgrDestroyOtherApp();

    sceIoMkdir("uma0:app", 0006);
    sceIoMkdir("uma0:appmeta", 0006);
    sceIoMkdir("uma0:license", 0006);
    sceIoMkdir("uma0:license/app", 0006);
    sceIoMkdir("uma0:app/VITAVAULT", 0006);
    sceIoMkdir("uma0:appmeta/VITAVAULT", 0006);
    sceIoMkdir("uma0:license/app/VITAVAULT", 0006);

    copyPath("ux0:app/VITAVAULT", "uma0:app/VITAVAULT", NULL);
    copyPath("ux0:appmeta/VITAVAULT", "uma0:appmeta/VITAVAULT", NULL);
    copyPath("ux0:license/app/VITAVAULT", "uma0:license/app/VITAVAULT", NULL);

    sceIoMkdir("uma0:data", 0777);
    sceIoMkdir("uma0:temp", 0006);

    shellUserRedirectUx0("sdstor0:uma-pp-act-a", "sdstor0:uma-lp-act-entire");

    vshIoUmount(0xF00, 0, 0, 0);

    remount(0x800);

    return 0;
}

// Unmount USB from ux0: (from VitaShell)
int umountUsbUx0() {
    sceAppMgrDestroyOtherApp();

    if (checkFileExist("sdstor0:xmc-lp-ign-userext"))
        shellUserRedirectUx0("sdstor0:xmc-lp-ign-userext", "sdstor0:xmc-lp-ign-userext");
    else
        shellUserRedirectUx0("sdstor0:int-lp-ign-userext", "sdstor0:int-lp-ign-userext");

    remount(0x800);

    remount(0xF00);

    return 0;
}
