/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope it will be useful but, except 
 * as otherwise stated in writing, without any warranty; without even the 
 * implied warranty of merchantability or fitness for a particular purpose. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK 
 *
 ******************************************************************************/

#if !defined(__OEMFUNCS_H__)
#define __OEMFUNCS_H__

#if defined (__cplusplus)
extern "C" {
#endif

typedef IMG_UINT32   (*PFN_SRV_BRIDGEDISPATCH)( IMG_UINT32  Ioctl,
												IMG_BYTE   *pInBuf,
												IMG_UINT32  InBufLen, 
											    IMG_BYTE   *pOutBuf,
												IMG_UINT32  OutBufLen,
												IMG_UINT32 *pdwBytesTransferred);
typedef struct PVRSRV_DC_OEM_JTABLE_TAG
{
	PFN_SRV_BRIDGEDISPATCH			pfnOEMBridgeDispatch;
	IMG_PVOID						pvDummy1;
	IMG_PVOID						pvDummy2;
	IMG_PVOID						pvDummy3;

} PVRSRV_DC_OEM_JTABLE;

#define OEM_GET_EXT_FUNCS			(1<<1)

#if defined(__cplusplus)
}
#endif

#endif	

