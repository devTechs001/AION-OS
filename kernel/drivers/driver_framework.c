/*
 * AION OS - Driver Framework Core
 * Unified driver management system
 */

#include <aion/drivers.h>
#include <aion/types.h>

// Initialize driver framework
bool init_driver_framework(void) {
    // Initialize PCI subsystem
    if (!init_pci_subsystem()) {
        return false;
    }
    
    // Initialize USB subsystem
    if (!init_usb_subsystem()) {
        return false;
    }
    
    // Initialize storage subsystem
    if (!init_storage_subsystem()) {
        return false;
    }
    
    return true;
}