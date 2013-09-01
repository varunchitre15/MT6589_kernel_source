###############################################################################
#
# Makefile for the cmmb drivers.
#
#
# Siano Mobile Silicon, Inc.
# MDTV receiver kernel modules.
# Copyright (C) 2006-2008, Uri Shkolnik
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################

ifeq ($(HAVE_CMMB_FEATURE),yes)
obj-$(CONFIG_MTK_CMMB)	:= cmmbdrv.o
cmmbdrv-objs :=smscoreapi.o  sms-cards.o smsendian.o smsir.o smschar.o smsspidrv.o smsspicommon.o smsspiphy_pxa.o 

# check the cmmb chip ,then use the matched firmware
ifeq ($(MTK_CMMB_CHIP),Siano_SMS2186)
ccflags-y += -DFLAG_CHIP_SMS2186
endif
ifeq ($(MTK_CMMB_CHIP),Siano_SMS3188)
ccflags-y += -DFLAG_CHIP_SMS3188
endif
endif
