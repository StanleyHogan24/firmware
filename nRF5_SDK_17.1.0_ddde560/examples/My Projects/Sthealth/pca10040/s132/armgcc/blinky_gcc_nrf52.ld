/* Linker script to configure memory regions. */

SEARCH_DIR(".")
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x26000, LENGTH = 0x58000
  RAM (rwx) : ORIGIN = 0x20005968, LENGTH = 0xa000
}

SECTIONS
{
  . = ALIGN(4);

  /* Dummy RAM Section */
  .mem_section_dummy_ram :
  {
  } > RAM

  /* Log Dynamic Data Section */
  .log_dynamic_data :
  {
    PROVIDE(__start_log_dynamic_data = .);
    KEEP(*(SORT(.log_dynamic_data*)))
    PROVIDE(__stop_log_dynamic_data = .);
  } > RAM

  /* Log Filter Data Section */
  .log_filter_data :
  {
    PROVIDE(__start_log_filter_data = .);
    KEEP(*(SORT(.log_filter_data*)))
    PROVIDE(__stop_log_filter_data = .);
  } > RAM

  /* Dummy ROM Section */
  .mem_section_dummy_rom :
  {
  } > FLASH

  /* BLE Observers Section */
  .sdh_ble_observers :
  {
    PROVIDE(__start_sdh_ble_observers = .);
    KEEP(*(SORT(.sdh_ble_observers*)))
    PROVIDE(__stop_sdh_ble_observers = .);
  } > FLASH

  /* SOC Observers Section */
  .sdh_soc_observers :
  {
    PROVIDE(__start_sdh_soc_observers = .);
    KEEP(*(SORT(.sdh_soc_observers*)))
    PROVIDE(__stop_sdh_soc_observers = .);
  } > FLASH

  /* SDH Request Observers Section */
  .sdh_req_observers :
  {
    PROVIDE(__start_sdh_req_observers = .);
    KEEP(*(SORT(.sdh_req_observers*)))
    PROVIDE(__stop_sdh_req_observers = .);
  } > FLASH

  /* SDH State Observers Section */
  .sdh_state_observers :
  {
    PROVIDE(__start_sdh_state_observers = .);
    KEEP(*(SORT(.sdh_state_observers*)))
    PROVIDE(__stop_sdh_state_observers = .);
  } > FLASH

  /* SDH Stack Observers Section */
  .sdh_stack_observers :
  {
    PROVIDE(__start_sdh_stack_observers = .);
    KEEP(*(SORT(.sdh_stack_observers*)))
    PROVIDE(__stop_sdh_stack_observers = .);
  } > FLASH

  /* Log Constant Data Section */
  .log_const_data :
  {
    PROVIDE(__start_log_const_data = .);
    KEEP(*(SORT(.log_const_data*)))
    PROVIDE(__stop_log_const_data = .);
  } > FLASH

  /* Log Backends Section */
  .log_backends :
  {
    PROVIDE(__start_log_backends = .);
    KEEP(*(SORT(.log_backends*)))
    PROVIDE(__stop_log_backends = .);
  } > FLASH

  /* NRF Balloc Section */
  .nrf_balloc :
  {
    PROVIDE(__start_nrf_balloc = .);
    KEEP(*(SORT(.nrf_balloc*)))
    PROVIDE(__stop_nrf_balloc = .);
  } > FLASH

  /* Add .data, .bss, and other standard sections */
  .data : AT (LOADADDR(.text) + SIZEOF(.text))
  {
    _sdata = .;
    *(.data)
    _edata = .;
  } > RAM

  .bss :
  {
    _sbss = .;
    *(.bss)
    *(COMMON)
    _ebss = .;
  } > RAM
}

