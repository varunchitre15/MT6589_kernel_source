#ifndef __CIRQ_H__
#define __CIRQ_H__

/*
 * Define hardware registers.
 */
#define  CIRQ_STA0   (0xF0204000)
#define  CIRQ_STA1   (0xF0204004)
#define  CIRQ_STA2   (0xF0204008)
#define  CIRQ_STA3   (0xF020400C)
#define  CIRQ_STA4   (0xF0204010)
#define  CIRQ_STA5   (0xF0204014)
#define  CIRQ_ACK0   (0xF0204040)
#define  CIRQ_ACK1   (0xF0204044)
#define  CIRQ_ACK2   (0xF0204048)
#define  CIRQ_ACK3   (0xF020404C)
#define  CIRQ_ACK4   (0xF0204050)
#define  CIRQ_ACK5   (0xF0204054)
#define  CIRQ_MASK0   (0xF0204080)
#define  CIRQ_MASK1   (0xF0204084)
#define  CIRQ_MASK2   (0xF0204088)
#define  CIRQ_MASK3   (0xF020408C)
#define  CIRQ_MASK4   (0xF0204090)
#define  CIRQ_MASK5   (0xF0204094)
#define  CIRQ_MASK_SET0   (0xF02040C0)
#define  CIRQ_MASK_SET1   (0xF02040C4)
#define  CIRQ_MASK_SET2   (0xF02040C8)
#define  CIRQ_MASK_SET3   (0xF02040CC)
#define  CIRQ_MASK_SET4   (0xF02040D0)
#define  CIRQ_MASK_SET5   (0xF02040D4)
#define  CIRQ_MASK_CLR0   (0xF0204100)
#define  CIRQ_MASK_CLR1   (0xF0204104)
#define  CIRQ_MASK_CLR2   (0xF0204108)
#define  CIRQ_MASK_CLR3   (0xF020410C)
#define  CIRQ_MASK_CLR4   (0xF0204110)
#define  CIRQ_MASK_CLR5   (0xF0204114)
#define  CIRQ_SENS0   (0xF0204140)
#define  CIRQ_SENS1   (0xF0204144)
#define  CIRQ_SENS2   (0xF0204148)
#define  CIRQ_SENS3   (0xF020414C)
#define  CIRQ_SENS4   (0xF0204150)
#define  CIRQ_SENS4   (0xF0204150)
#define  CIRQ_SENS5   (0xF0204154)
#define  CIRQ_SENS_SET0   (0xF0204180)
#define  CIRQ_SENS_SET1   (0xF0204184)
#define  CIRQ_SENS_SET2   (0xF0204188)
#define  CIRQ_SENS_SET3   (0xF020418C)
#define  CIRQ_SENS_SET4   (0xF0204190)
#define  CIRQ_SENS_SET5   (0xF0204194)
#define  CIRQ_SENS_CLR0   (0xF02041C0)
#define  CIRQ_SENS_CLR1   (0xF02041C4)
#define  CIRQ_SENS_CLR2   (0xF02041C8)
#define  CIRQ_SENS_CLR3   (0xF02041CC)
#define  CIRQ_SENS_CLR4   (0xF02041D0)
#define  CIRQ_SENS_CLR5   (0xF02041D4)
#define  CIRQ_POL0   (0xF0204200)
#define  CIRQ_POL1   (0xF0204204)
#define  CIRQ_POL2   (0xF0204208)
#define  CIRQ_POL3   (0xF020420C)
#define  CIRQ_POL4   (0xF0204210)
#define  CIRQ_POL5   (0xF0204214)
#define  CIRQ_POL_SET0   (0xF0204240)
#define  CIRQ_POL_SET1   (0xF0204244)
#define  CIRQ_POL_SET2   (0xF0204248)
#define  CIRQ_POL_SET3   (0xF020424C)
#define  CIRQ_POL_SET4   (0xF0204250)
#define  CIRQ_POL_SET5   (0xF0204254)
#define  CIRQ_POL_CLR0   (0xF0204280)
#define  CIRQ_POL_CLR1   (0xF0204284)
#define  CIRQ_POL_CLR2   (0xF0204288)
#define  CIRQ_POL_CLR3   (0xF020428C)
#define  CIRQ_POL_CLR4   (0xF0204290)
#define  CIRQ_POL_CLR5   (0xF0204294)
#define  CIRQ_CON   (0xF0204300)



/*
 * Define constants.
 */
#define MT_CIRQ_POL_NEG (0)
#define MT_CIRQ_POL_POS (1)
#define MT_EDGE_SENSITIVE (0)
#define MT_LEVEL_SENSITIVE (1)


/*
 * Define function prototypes.
 */
void mt_cirq_wfi_func(void);
void mt_cirq_enable(void);
void mt_cirq_disable(void);
void mt_cirq_clone_gic(void);
void mt_cirq_flush(void);
#endif  /*!__CIRQ_H__ */
