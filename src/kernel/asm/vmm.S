# croOS vmm.S - Virtual memory assembly helpers
# vmm_load_directory: load CR3 with page directory
# vmm_flush_tlb: flush TLB by reloading CR3

.global vmm_load_directory
.global vmm_flush_tlb

vmm_load_directory:
    mov 4(%esp), %eax
    mov %eax, %cr3
    ret

vmm_flush_tlb:
    mov %cr3, %eax
    mov %eax, %cr3
    ret
