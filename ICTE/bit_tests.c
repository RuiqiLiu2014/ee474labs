#include <assert.h>
#include <stdint.h>
#include <stdio.h>

/* ── Declarations (provide your own implementations) ─────────────────────── */

// uint16_t* bit_set16(uint16_t *register, uint16_t bits);
// uint32_t* bit_set32(uint32_t *register, uint32_t bits);
// uint16_t* bit_clear16(uint16_t *register, uint16_t bits);
// uint32_t* bit_clear32(uint32_t *register, uint32_t bits);

void bit_set16(uint16_t *reg, uint16_t bits){
  *reg = *reg | bits;
}
 
void bit_set32(uint32_t *reg, uint32_t bits){
  *reg = *reg | bits;
}

void bit_clear16(uint16_t *reg, uint16_t bits){
  *reg = *reg & ~bits;
}
 
void bit_clear32(uint32_t *reg, uint32_t bits){
  *reg = *reg & ~bits;
}

/* ── Tests ───────────────────────────────────────────────────────────────── */

void test_bit_set16(void) {
    uint16_t reg;

    /* Set specific bits into a zeroed register */
    reg = 0x0000;
    bit_set16(&reg, 0x000F);
    assert(reg == 0x000F);

    /* OR new bits into an already-populated register (existing bits preserved) */
    reg = 0x00F0;
    bit_set16(&reg, 0x000F);
    assert(reg == 0x00FF);

    printf("test_bit_set16 passed\n");
}

void test_bit_set32(void) {
    uint32_t reg;

    /* Set specific bits into a zeroed register */
    reg = 0x00000000;
    bit_set32(&reg, 0x0000000F);
    assert(reg == 0x0000000F);

    /* OR new bits into an already-populated register (existing bits preserved) */
    reg = 0x00F00000;
    bit_set32(&reg, 0x000F0000);
    assert(reg == 0x00FF0000);

    printf("test_bit_set32 passed\n");
}

void test_bit_clear16(void) {
    uint16_t reg;

    /* Clear specific bits from a fully set register */
    reg = 0xFFFF;
    bit_clear16(&reg, 0x000F);
    assert(reg == 0xFFF0);

    /* Clear bits leaving unrelated bits untouched */
    reg = 0x00FF;
    bit_clear16(&reg, 0x000F);
    assert(reg == 0x00F0);

    /* Clearing already-clear bits changes nothing */
    reg = 0x00F0;
    bit_clear16(&reg, 0x000F);
    assert(reg == 0x00F0);

    printf("test_bit_clear16 passed\n");
}

void test_bit_clear32(void) {
    uint32_t reg;

    /* Clear specific bits from a fully set register */
    reg = 0xFFFFFFFF;
    bit_clear32(&reg, 0x0000000F);
    assert(reg == 0xFFFFFFF0);

    /* Clear bits leaving unrelated bits untouched */
    reg = 0x00FF0000;
    bit_clear32(&reg, 0x000F0000);
    assert(reg == 0x00F00000);

    /* Clearing already-clear bits changes nothing */
    reg = 0x00F00000;
    bit_clear32(&reg, 0x000F0000);
    assert(reg == 0x00F00000);

    printf("test_bit_clear32 passed\n");
}

/* ── Main ────────────────────────────────────────────────────────────────── */

int main(void) {
    test_bit_set16();
    test_bit_set32();
    test_bit_clear16();
    test_bit_clear32();
    return 0;
}

