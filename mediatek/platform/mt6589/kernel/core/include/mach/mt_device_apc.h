


#ifndef _MTK_DEVICE_APC_H
#define _MTK_DEVICE_APC_H


#define DEVAPC0_AO_BASE         0xF0010000      // for AP
#define DEVAPC0_PD_BASE         0xF0207000      // for AP
#define DEVAPC1_AO_BASE         0xF0010100      // for AP
#define DEVAPC1_PD_BASE         0xF0207100      // for AP
#define DEVAPC2_AO_BASE         0xF0010200      // for AP
#define DEVAPC2_PD_BASE         0xF0207200      // for AP
#define DEVAPC3_AO_BASE         0xF0010300      // for MM
#define DEVAPC3_PD_BASE         0xF0207300      // for MM
#define DEVAPC4_AO_BASE         0xF0010400      // for MM
#define DEVAPC4_PD_BASE         0xF0207400      // for MM



#define DEVAPC0_D0_APC_0		    ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0000))
#define DEVAPC0_D0_APC_1            ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0004))
#define DEVAPC0_D1_APC_0            ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0008))
#define DEVAPC0_D1_APC_1            ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x000C))
#define DEVAPC0_D2_APC_0            ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0010))
#define DEVAPC0_D2_APC_1            ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0014))
#define DEVAPC0_D3_APC_0            ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0018))
#define DEVAPC0_D3_APC_1            ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x001C))
#define DEVAPC0_D0_VIO_MASK         ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x0020))
#define DEVAPC0_D1_VIO_MASK         ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x0024))
#define DEVAPC0_D2_VIO_MASK         ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x0028))
#define DEVAPC0_D3_VIO_MASK         ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x002C))
#define DEVAPC0_D0_VIO_STA          ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x0030))
#define DEVAPC0_D1_VIO_STA          ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x0034))
#define DEVAPC0_D2_VIO_STA          ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x0038))
#define DEVAPC0_D3_VIO_STA          ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x003C))
#define DEVAPC0_VIO_DBG0            ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x0040))
#define DEVAPC0_VIO_DBG1            ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x0044))
#define DEVAPC0_DXS_VIO_MASK        ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x0080))
#define DEVAPC0_DXS_VIO_STA         ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x0084))
#define DEVAPC0_APC_CON             ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0090))
#define DEVAPC0_PD_APC_CON          ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x0090))
#define DEVAPC0_APC_LOCK            ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0094))
#define DEVAPC0_MAS_DOM             ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x00A0))
#define DEVAPC0_MAS_SEC             ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x00A4))
#define DEVAPC0_DEC_ERR_CON         ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x00B4))
#define DEVAPC0_DEC_ERR_ADDR        ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x00B8))
#define DEVAPC0_DEC_ERR_ID          ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x00BC))

                                                                      
#define DEVAPC1_D0_APC_0		    ((volatile unsigned int*)(DEVAPC1_AO_BASE+0x0000))
#define DEVAPC1_D0_APC_1            ((volatile unsigned int*)(DEVAPC1_AO_BASE+0x0004))
#define DEVAPC1_D1_APC_0            ((volatile unsigned int*)(DEVAPC1_AO_BASE+0x0008))
#define DEVAPC1_D1_APC_1            ((volatile unsigned int*)(DEVAPC1_AO_BASE+0x000C))
#define DEVAPC1_D2_APC_0            ((volatile unsigned int*)(DEVAPC1_AO_BASE+0x0010))
#define DEVAPC1_D2_APC_1            ((volatile unsigned int*)(DEVAPC1_AO_BASE+0x0014))
#define DEVAPC1_D3_APC_0            ((volatile unsigned int*)(DEVAPC1_AO_BASE+0x0018))
#define DEVAPC1_D3_APC_1            ((volatile unsigned int*)(DEVAPC1_AO_BASE+0x001C))
#define DEVAPC1_D0_VIO_MASK         ((volatile unsigned int*)(DEVAPC1_PD_BASE+0x0020))
#define DEVAPC1_D1_VIO_MASK         ((volatile unsigned int*)(DEVAPC1_PD_BASE+0x0024))
#define DEVAPC1_D2_VIO_MASK         ((volatile unsigned int*)(DEVAPC1_PD_BASE+0x0028))
#define DEVAPC1_D3_VIO_MASK         ((volatile unsigned int*)(DEVAPC1_PD_BASE+0x002C))
#define DEVAPC1_D0_VIO_STA          ((volatile unsigned int*)(DEVAPC1_PD_BASE+0x0030))
#define DEVAPC1_D1_VIO_STA          ((volatile unsigned int*)(DEVAPC1_PD_BASE+0x0034))
#define DEVAPC1_D2_VIO_STA          ((volatile unsigned int*)(DEVAPC1_PD_BASE+0x0038))
#define DEVAPC1_D3_VIO_STA          ((volatile unsigned int*)(DEVAPC1_PD_BASE+0x003C))
#define DEVAPC1_VIO_DBG0            ((volatile unsigned int*)(DEVAPC1_PD_BASE+0x0040))
#define DEVAPC1_VIO_DBG1            ((volatile unsigned int*)(DEVAPC1_PD_BASE+0x0044))
#define DEVAPC1_DXS_VIO_MASK        ((volatile unsigned int*)(DEVAPC1_PD_BASE+0x0080))
#define DEVAPC1_DXS_VIO_STA         ((volatile unsigned int*)(DEVAPC1_PD_BASE+0x0084))
#define DEVAPC1_APC_CON             ((volatile unsigned int*)(DEVAPC1_AO_BASE+0x0090))
#define DEVAPC1_PD_APC_CON          ((volatile unsigned int*)(DEVAPC1_PD_BASE+0x0090))
#define DEVAPC1_APC_LOCK            ((volatile unsigned int*)(DEVAPC1_AO_BASE+0x0094))
#define DEVAPC1_MAS_DOM             ((volatile unsigned int*)(DEVAPC1_AO_BASE+0x00A0))
#define DEVAPC1_MAS_SEC             ((volatile unsigned int*)(DEVAPC1_AO_BASE+0x00A4))
#define DEVAPC1_DEC_ERR_CON         ((volatile unsigned int*)(DEVAPC1_PD_BASE+0x00B4))
#define DEVAPC1_DEC_ERR_ADDR        ((volatile unsigned int*)(DEVAPC1_PD_BASE+0x00B8))
#define DEVAPC1_DEC_ERR_ID          ((volatile unsigned int*)(DEVAPC1_PD_BASE+0x00BC))


#define DEVAPC2_D0_APC_0		    ((volatile unsigned int*)(DEVAPC2_AO_BASE+0x0000))
#define DEVAPC2_D0_APC_1            ((volatile unsigned int*)(DEVAPC2_AO_BASE+0x0004))
#define DEVAPC2_D1_APC_0            ((volatile unsigned int*)(DEVAPC2_AO_BASE+0x0008))
#define DEVAPC2_D1_APC_1            ((volatile unsigned int*)(DEVAPC2_AO_BASE+0x000C))
#define DEVAPC2_D2_APC_0            ((volatile unsigned int*)(DEVAPC2_AO_BASE+0x0010))
#define DEVAPC2_D2_APC_1            ((volatile unsigned int*)(DEVAPC2_AO_BASE+0x0014))
#define DEVAPC2_D3_APC_0            ((volatile unsigned int*)(DEVAPC2_AO_BASE+0x0018))
#define DEVAPC2_D3_APC_1            ((volatile unsigned int*)(DEVAPC2_AO_BASE+0x001C))
#define DEVAPC2_D0_VIO_MASK         ((volatile unsigned int*)(DEVAPC2_PD_BASE+0x0020))
#define DEVAPC2_D1_VIO_MASK         ((volatile unsigned int*)(DEVAPC2_PD_BASE+0x0024))
#define DEVAPC2_D2_VIO_MASK         ((volatile unsigned int*)(DEVAPC2_PD_BASE+0x0028))
#define DEVAPC2_D3_VIO_MASK         ((volatile unsigned int*)(DEVAPC2_PD_BASE+0x002C))
#define DEVAPC2_D0_VIO_STA          ((volatile unsigned int*)(DEVAPC2_PD_BASE+0x0030))
#define DEVAPC2_D1_VIO_STA          ((volatile unsigned int*)(DEVAPC2_PD_BASE+0x0034))
#define DEVAPC2_D2_VIO_STA          ((volatile unsigned int*)(DEVAPC2_PD_BASE+0x0038))
#define DEVAPC2_D3_VIO_STA          ((volatile unsigned int*)(DEVAPC2_PD_BASE+0x003C))
#define DEVAPC2_VIO_DBG0            ((volatile unsigned int*)(DEVAPC2_PD_BASE+0x0040))
#define DEVAPC2_VIO_DBG1            ((volatile unsigned int*)(DEVAPC2_PD_BASE+0x0044))
#define DEVAPC2_DXS_VIO_MASK        ((volatile unsigned int*)(DEVAPC2_PD_BASE+0x0080))
#define DEVAPC2_DXS_VIO_STA         ((volatile unsigned int*)(DEVAPC2_PD_BASE+0x0084))
#define DEVAPC2_APC_CON             ((volatile unsigned int*)(DEVAPC2_AO_BASE+0x0090))
#define DEVAPC2_PD_APC_CON          ((volatile unsigned int*)(DEVAPC2_PD_BASE+0x0090))
#define DEVAPC2_APC_LOCK            ((volatile unsigned int*)(DEVAPC2_AO_BASE+0x0094))
#define DEVAPC2_MAS_DOM             ((volatile unsigned int*)(DEVAPC2_AO_BASE+0x00A0))
#define DEVAPC2_MAS_SEC             ((volatile unsigned int*)(DEVAPC2_AO_BASE+0x00A4))
#define DEVAPC2_DEC_ERR_CON         ((volatile unsigned int*)(DEVAPC2_PD_BASE+0x00B4))
#define DEVAPC2_DEC_ERR_ADDR        ((volatile unsigned int*)(DEVAPC2_PD_BASE+0x00B8))
#define DEVAPC2_DEC_ERR_ID          ((volatile unsigned int*)(DEVAPC2_PD_BASE+0x00BC))


#define DEVAPC3_D0_APC_0		    ((volatile unsigned int*)(DEVAPC3_AO_BASE+0x0000))
#define DEVAPC3_D0_APC_1            ((volatile unsigned int*)(DEVAPC3_AO_BASE+0x0004))
#define DEVAPC3_D1_APC_0            ((volatile unsigned int*)(DEVAPC3_AO_BASE+0x0008))
#define DEVAPC3_D1_APC_1            ((volatile unsigned int*)(DEVAPC3_AO_BASE+0x000C))
#define DEVAPC3_D2_APC_0            ((volatile unsigned int*)(DEVAPC3_AO_BASE+0x0010))
#define DEVAPC3_D2_APC_1            ((volatile unsigned int*)(DEVAPC3_AO_BASE+0x0014))
#define DEVAPC3_D3_APC_0            ((volatile unsigned int*)(DEVAPC3_AO_BASE+0x0018))
#define DEVAPC3_D3_APC_1            ((volatile unsigned int*)(DEVAPC3_AO_BASE+0x001C))
#define DEVAPC3_D0_VIO_MASK         ((volatile unsigned int*)(DEVAPC3_PD_BASE+0x0020))
#define DEVAPC3_D1_VIO_MASK         ((volatile unsigned int*)(DEVAPC3_PD_BASE+0x0024))
#define DEVAPC3_D2_VIO_MASK         ((volatile unsigned int*)(DEVAPC3_PD_BASE+0x0028))
#define DEVAPC3_D3_VIO_MASK         ((volatile unsigned int*)(DEVAPC3_PD_BASE+0x002C))
#define DEVAPC3_D0_VIO_STA          ((volatile unsigned int*)(DEVAPC3_PD_BASE+0x0030))
#define DEVAPC3_D1_VIO_STA          ((volatile unsigned int*)(DEVAPC3_PD_BASE+0x0034))
#define DEVAPC3_D2_VIO_STA          ((volatile unsigned int*)(DEVAPC3_PD_BASE+0x0038))
#define DEVAPC3_D3_VIO_STA          ((volatile unsigned int*)(DEVAPC3_PD_BASE+0x003C))
#define DEVAPC3_VIO_DBG0            ((volatile unsigned int*)(DEVAPC3_PD_BASE+0x0040))
#define DEVAPC3_VIO_DBG1            ((volatile unsigned int*)(DEVAPC3_PD_BASE+0x0044))
#define DEVAPC3_DXS_VIO_MASK        ((volatile unsigned int*)(DEVAPC3_PD_BASE+0x0080))
#define DEVAPC3_DXS_VIO_STA         ((volatile unsigned int*)(DEVAPC3_PD_BASE+0x0084))
#define DEVAPC3_APC_CON             ((volatile unsigned int*)(DEVAPC3_AO_BASE+0x0090))
#define DEVAPC3_PD_APC_CON          ((volatile unsigned int*)(DEVAPC3_PD_BASE+0x0090))
#define DEVAPC3_APC_LOCK            ((volatile unsigned int*)(DEVAPC3_AO_BASE+0x0094))
#define DEVAPC3_MAS_DOM             ((volatile unsigned int*)(DEVAPC3_AO_BASE+0x00A0))
#define DEVAPC3_MAS_SEC             ((volatile unsigned int*)(DEVAPC3_AO_BASE+0x00A4))
#define DEVAPC3_DEC_ERR_CON         ((volatile unsigned int*)(DEVAPC3_PD_BASE+0x00B4))
#define DEVAPC3_DEC_ERR_ADDR        ((volatile unsigned int*)(DEVAPC3_PD_BASE+0x00B8))
#define DEVAPC3_DEC_ERR_ID          ((volatile unsigned int*)(DEVAPC3_PD_BASE+0x00BC))


#define DEVAPC4_D0_APC_0		    ((volatile unsigned int*)(DEVAPC4_AO_BASE+0x0000))
#define DEVAPC4_D0_APC_1            ((volatile unsigned int*)(DEVAPC4_AO_BASE+0x0004))
#define DEVAPC4_D1_APC_0            ((volatile unsigned int*)(DEVAPC4_AO_BASE+0x0008))
#define DEVAPC4_D1_APC_1            ((volatile unsigned int*)(DEVAPC4_AO_BASE+0x000C))
#define DEVAPC4_D2_APC_0            ((volatile unsigned int*)(DEVAPC4_AO_BASE+0x0010))
#define DEVAPC4_D2_APC_1            ((volatile unsigned int*)(DEVAPC4_AO_BASE+0x0014))
#define DEVAPC4_D3_APC_0            ((volatile unsigned int*)(DEVAPC4_AO_BASE+0x0018))
#define DEVAPC4_D3_APC_1            ((volatile unsigned int*)(DEVAPC4_AO_BASE+0x001C))
#define DEVAPC4_D0_VIO_MASK         ((volatile unsigned int*)(DEVAPC4_PD_BASE+0x0020))
#define DEVAPC4_D1_VIO_MASK         ((volatile unsigned int*)(DEVAPC4_PD_BASE+0x0024))
#define DEVAPC4_D2_VIO_MASK         ((volatile unsigned int*)(DEVAPC4_PD_BASE+0x0028))
#define DEVAPC4_D3_VIO_MASK         ((volatile unsigned int*)(DEVAPC4_PD_BASE+0x002C))
#define DEVAPC4_D0_VIO_STA          ((volatile unsigned int*)(DEVAPC4_PD_BASE+0x0030))
#define DEVAPC4_D1_VIO_STA          ((volatile unsigned int*)(DEVAPC4_PD_BASE+0x0034))
#define DEVAPC4_D2_VIO_STA          ((volatile unsigned int*)(DEVAPC4_PD_BASE+0x0038))
#define DEVAPC4_D3_VIO_STA          ((volatile unsigned int*)(DEVAPC4_PD_BASE+0x003C))
#define DEVAPC4_VIO_DBG0            ((volatile unsigned int*)(DEVAPC4_PD_BASE+0x0040))
#define DEVAPC4_VIO_DBG1            ((volatile unsigned int*)(DEVAPC4_PD_BASE+0x0044))
#define DEVAPC4_DXS_VIO_MASK        ((volatile unsigned int*)(DEVAPC4_PD_BASE+0x0080))
#define DEVAPC4_DXS_VIO_STA         ((volatile unsigned int*)(DEVAPC4_PD_BASE+0x0084))
#define DEVAPC4_APC_CON             ((volatile unsigned int*)(DEVAPC4_AO_BASE+0x0090))
#define DEVAPC4_PD_APC_CON          ((volatile unsigned int*)(DEVAPC4_PD_BASE+0x0090))
#define DEVAPC4_APC_LOCK            ((volatile unsigned int*)(DEVAPC4_AO_BASE+0x0094))
#define DEVAPC4_MAS_DOM             ((volatile unsigned int*)(DEVAPC4_AO_BASE+0x00A0))
#define DEVAPC4_MAS_SEC             ((volatile unsigned int*)(DEVAPC4_AO_BASE+0x00A4))
#define DEVAPC4_DEC_ERR_CON         ((volatile unsigned int*)(DEVAPC4_PD_BASE+0x00B4))
#define DEVAPC4_DEC_ERR_ADDR        ((volatile unsigned int*)(DEVAPC4_PD_BASE+0x00B8))
#define DEVAPC4_DEC_ERR_ID          ((volatile unsigned int*)(DEVAPC4_PD_BASE+0x00BC))


/* DOMAIN_SETUP */
#define DOMAIN_AP						0
#define DOMAIN_MD1	    				1
#define DOMAIN_MD2						2
#define DOMAIN_MM                       3


#endif
