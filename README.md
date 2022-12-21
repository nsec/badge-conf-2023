Nsec 2023 Conference Badge Test Firmware
----------------------------------------

Run this once on a fresh board to set the fuses:

    pio run -e program_via_AVRISP -t fuses

To flash the firmware:

    pio run -e program_via_AVRISP -t upload

To use clangd language server:

    pio run -t compiledb
