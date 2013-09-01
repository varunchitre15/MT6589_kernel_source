if [ "$#" -eq 0 ]; then

	echo "-----------Select Key Config------------"
	echo " 1 = MTK SIGN"
	echo " 2 = Reserve"
	echo " 3 = BROWN SIGN (FULL SECU ENABLE)"
	echo " 4 = Reserve"	
	echo " 5 = BROWN SIGN (SECU DISABLE)"
	echo " 6 = RED SIGN (FULL SECU ENABLE)"		
	echo "--------------------------------------------"
	echo
	read -p "Please input Q Exit: " yn

	if [ "$yn" == "q"  ] || [ "$yn" == "Q"  ]; then
		exit 0	
	fi
else
	yn=$1
fi

	if [ "$yn" == "1" ]; then

		sed -i 's/^\(MTK_SEC_BOOT\s*=\s*\).*$/\1ATTR_SBOOT_ONLY_ENABLE_ON_SCHIP/' 					./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_CHIP_SUPPORT\s*=\s*\).*$/\1yes/' 	./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_AUTH\s*=\s*\).*$/\1no/' 		./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_ENCODE\s*=\s*\).*$/\1no/'	./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_NVRAM_ANTI_CLONE\s*=\s*\).*$/\1no/' ./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_SECRO_AC_SUPPORT\s*=\s*\).*$/\1no/' 				./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_USBDL\s*=\s*\).*$/\1ATTR_SUSBDL_ONLY_ENABLE_ON_SCHIP/' 									./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MP_KEY\s*=\s*\).*$/\1KEY_TYPE_MTK/' 									./mediatek/config/common/ProjectConfig.mk

		sed -i 's/^\(MTK_SEC_BOOT\s*=\s*\).*$/\1ATTR_SBOOT_ONLY_ENABLE_ON_SCHIP/' 					./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_CHIP_SUPPORT\s*=\s*\).*$/\1yes/' 	./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_AUTH\s*=\s*\).*$/\1no/' 		./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_ENCODE\s*=\s*\).*$/\1no/'	./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_NVRAM_ANTI_CLONE\s*=\s*\).*$/\1no/' ./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_SECRO_AC_SUPPORT\s*=\s*\).*$/\1yes/' 				./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_USBDL\s*=\s*\).*$/\1ATTR_SUSBDL_ONLY_ENABLE_ON_SCHIP/' 									./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MP_KEY\s*=\s*\).*$/\1KEY_TYPE_MTK/' 									./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/mtk/image_auth/IMG_AUTH_CFG.ini			./mediatek/custom/arima89_we_s_jb2/security/image_auth/IMG_AUTH_CFG.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/mtk/image_auth/IMG_AUTH_KEY.ini                   ./mediatek/custom/arima89_we_s_jb2/security/image_auth/IMG_AUTH_KEY.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/mtk/chip_config/s/key/CHIP_TEST_KEY.ini           ./mediatek/custom/arima89_we_s_jb2/security/chip_config/s/key/CHIP_TEST_KEY.ini      
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/mtk/chip_config/s/cfg/SECURE_JTAG_CONFIG.ini      ./mediatek/custom/arima89_we_s_jb2/security/chip_config/s/cfg/SECURE_JTAG_CONFIG.ini 
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/mtk/chip_config/s/cfg/CHIP_CONFIG.ini             ./mediatek/custom/arima89_we_s_jb2/security/chip_config/s/cfg/CHIP_CONFIG.ini        
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/mtk/sec_file_list/SFL_CFG.ini                     ./mediatek/custom/arima89_we_s_jb2/security/sec_file_list/SFL_CFG.ini                
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/mtk/sml_auth/SML_AUTH_CFG.ini                     ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SML_AUTH_CFG.ini                
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/mtk/sml_auth/SDS_AUTH_CFG.ini                     ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SDS_AUTH_CFG.ini                
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/mtk/sml_auth/SDS_ENCODE_CFG.ini                   ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SDS_ENCODE_CFG.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/mtk/sml_auth/SML_ENCODE_KEY.ini                   ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SML_ENCODE_KEY.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/mtk/sml_auth/SML_AUTH_KEY.ini                     ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SML_AUTH_KEY.ini                
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/mtk/sml_auth/SML_ENCODE_CFG.ini                   ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SML_ENCODE_CFG.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/mtk/image_encrypt/IMG_ENCRYPT_CFG.ini             ./mediatek/custom/arima89_we_s_jb2/security/image_encrypt/IMG_ENCRYPT_CFG.ini        
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/mtk/image_encrypt/IMG_ENCRYPT_KEY.ini             ./mediatek/custom/arima89_we_s_jb2/security/image_encrypt/IMG_ENCRYPT_KEY.ini        
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/mtk/recovery/SEC_VER.txt           			 ./mediatek/custom/arima89_we_s_jb2/security/recovery/SEC_VER.txt       		
		echo "Copied MTK key settings to the target."
		
		echo rm ./mediatek/custom/arima89_we_s_jb2/security/sec_file_list/SECRO_SFL.ini              
		echo rm ./mediatek/custom/arima89_we_s_jb2/security/sec_file_list/ANDRO_SFL.ini              
		echo echo "[key transfer]Delete done SECRO_SFL.in and ANDRO_SFL.inii."
		echo cp -f ./mediatek/external/seclib/S_ANDRO_SFL.ini.signed.dev05 ./mediatek/external/seclib/S_ANDRO_SFL.ini
		echo cp -f ./mediatek/platform/mt6577/kernel/core/include/mach/sbchk_base.h.arima89_we_s_jb2 ./mediatek/platform/mt6577/kernel/core/include/mach/sbchk_base.h
	
		
	elif [ "$yn" == "3" ]; then
		echo MTK_SEC_BOOT=ATTR_SBOOT_ENABLE
		echo MTK_SEC_CHIP_SUPPORT=yes
		echo MTK_SEC_MODEM_AUTH=yes
		echo MTK_SEC_MODEM_ENCODE=yes
		echo MTK_SEC_MODEM_NVRAM_ANTI_CLONE=yes
		echo MTK_SEC_SECRO_AC_SUPPORT=yes
		echo MTK_SEC_USBDL=ATTR_SUSBDL_ENABLE

		sed -i 's/^\(MTK_SEC_BOOT\s*=\s*\).*$/\1ATTR_SBOOT_ENABLE/' 					./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_CHIP_SUPPORT\s*=\s*\).*$/\1yes/' 	./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_AUTH\s*=\s*\).*$/\1yes/' 		./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_ENCODE\s*=\s*\).*$/\1yes/'	./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_NVRAM_ANTI_CLONE\s*=\s*\).*$/\1yes/' ./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_SECRO_AC_SUPPORT\s*=\s*\).*$/\1yes/' 				./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_USBDL\s*=\s*\).*$/\1ATTR_SUSBDL_ENABLE/' 									./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MP_KEY\s*=\s*\).*$/\1KEY_TYPE_BROWN/' 									./mediatek/config/common/ProjectConfig.mk
			
		sed -i 's/^\(MTK_SEC_BOOT\s*=\s*\).*$/\1ATTR_SBOOT_ENABLE/' 					./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_CHIP_SUPPORT\s*=\s*\).*$/\1yes/' 	./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_AUTH\s*=\s*\).*$/\1yes/' 		./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_ENCODE\s*=\s*\).*$/\1yes/'	./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_NVRAM_ANTI_CLONE\s*=\s*\).*$/\1yes/' ./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_SECRO_AC_SUPPORT\s*=\s*\).*$/\1yes/' 				./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_USBDL\s*=\s*\).*$/\1ATTR_SUSBDL_ENABLE/' 									./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MP_KEY\s*=\s*\).*$/\1KEY_TYPE_BROWN/' 									./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk

		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/image_auth/IMG_AUTH_CFG.ini		   	  ./mediatek/custom/arima89_we_s_jb2/security/image_auth/IMG_AUTH_CFG.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/image_auth/IMG_AUTH_KEY.ini                   ./mediatek/custom/arima89_we_s_jb2/security/image_auth/IMG_AUTH_KEY.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/chip_config/s/key/CHIP_TEST_KEY.ini           ./mediatek/custom/arima89_we_s_jb2/security/chip_config/s/key/CHIP_TEST_KEY.ini      
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/chip_config/s/cfg/SECURE_JTAG_CONFIG.ini      ./mediatek/custom/arima89_we_s_jb2/security/chip_config/s/cfg/SECURE_JTAG_CONFIG.ini 
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/chip_config/s/cfg/CHIP_CONFIG.ini             ./mediatek/custom/arima89_we_s_jb2/security/chip_config/s/cfg/CHIP_CONFIG.ini        
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/sec_file_list/SFL_CFG.ini                     ./mediatek/custom/arima89_we_s_jb2/security/sec_file_list/SFL_CFG.ini                
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/sml_auth/SML_AUTH_CFG.ini                     ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SML_AUTH_CFG.ini                
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/sml_auth/SDS_AUTH_CFG.ini                     ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SDS_AUTH_CFG.ini                
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/sml_auth/SDS_ENCODE_CFG.ini                   ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SDS_ENCODE_CFG.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/sml_auth/SML_ENCODE_KEY.ini                   ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SML_ENCODE_KEY.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/sml_auth/SML_AUTH_KEY.ini                     ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SML_AUTH_KEY.ini                
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/sml_auth/SML_ENCODE_CFG.ini                   ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SML_ENCODE_CFG.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/image_encrypt/IMG_ENCRYPT_CFG.ini             ./mediatek/custom/arima89_we_s_jb2/security/image_encrypt/IMG_ENCRYPT_CFG.ini        
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/image_encrypt/IMG_ENCRYPT_KEY.ini             ./mediatek/custom/arima89_we_s_jb2/security/image_encrypt/IMG_ENCRYPT_KEY.ini        
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/recovery/SEC_VER.txt 			             ./mediatek/custom/arima89_we_s_jb2/security/recovery/SEC_VER.txt        		
		echo "[key transfer]Copied MTK key settings to the target."
		
		echo rm ./mediatek/custom/arima89_we_s_jb2/security/sec_file_list/SECRO_SFL.ini              
		echo rm ./mediatek/custom/arima89_we_s_jb2/security/sec_file_list/ANDRO_SFL.ini              
		echo echo "[key transfer]Delete done SECRO_SFL.in and ANDRO_SFL.inii."
		echo cp -f ./mediatek/external/seclib/S_ANDRO_SFL.ini.signed.mtk ./mediatek/external/seclib/S_ANDRO_SFL.ini
		echo cp -f ./mediatek/platform/mt6577/kernel/core/include/mach/sbchk_base.h.arima89_we_s_jb2 ./mediatek/platform/mt6577/kernel/core/include/mach/sbchk_base.h

	elif [ "$yn" == "5" ]; then

		sed -i 's/^\(MTK_SEC_BOOT\s*=\s*\).*$/\1ATTR_SBOOT_ONLY_ENABLE_ON_SCHIP/' 					./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_CHIP_SUPPORT\s*=\s*\).*$/\1yes/' 	./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_AUTH\s*=\s*\).*$/\1no/' 		./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_ENCODE\s*=\s*\).*$/\1no/'	./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_NVRAM_ANTI_CLONE\s*=\s*\).*$/\1no/' ./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_SECRO_AC_SUPPORT\s*=\s*\).*$/\1no/' 				./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_USBDL\s*=\s*\).*$/\1ATTR_SUSBDL_ONLY_ENABLE_ON_SCHIP/' 									./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MP_KEY\s*=\s*\).*$/\1KEY_TYPE_BROWN/' 									./mediatek/config/common/ProjectConfig.mk

		sed -i 's/^\(MTK_SEC_BOOT\s*=\s*\).*$/\1ATTR_SBOOT_ONLY_ENABLE_ON_SCHIP/' 					./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_CHIP_SUPPORT\s*=\s*\).*$/\1yes/' 	./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_AUTH\s*=\s*\).*$/\1no/' 		./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_ENCODE\s*=\s*\).*$/\1no/'	./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_NVRAM_ANTI_CLONE\s*=\s*\).*$/\1no/' ./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_SECRO_AC_SUPPORT\s*=\s*\).*$/\1yes/' 				./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_USBDL\s*=\s*\).*$/\1ATTR_SUSBDL_ONLY_ENABLE_ON_SCHIP/' 									./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MP_KEY\s*=\s*\).*$/\1KEY_TYPE_BROWN/' 									./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk

		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/image_auth/IMG_AUTH_CFG.ini		   	  ./mediatek/custom/arima89_we_s_jb2/security/image_auth/IMG_AUTH_CFG.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/image_auth/IMG_AUTH_KEY.ini                   ./mediatek/custom/arima89_we_s_jb2/security/image_auth/IMG_AUTH_KEY.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/chip_config/s/key/CHIP_TEST_KEY.ini           ./mediatek/custom/arima89_we_s_jb2/security/chip_config/s/key/CHIP_TEST_KEY.ini      
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/chip_config/s/cfg/SECURE_JTAG_CONFIG.ini      ./mediatek/custom/arima89_we_s_jb2/security/chip_config/s/cfg/SECURE_JTAG_CONFIG.ini 
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/chip_config/s/cfg/CHIP_CONFIG.ini             ./mediatek/custom/arima89_we_s_jb2/security/chip_config/s/cfg/CHIP_CONFIG.ini        
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/sec_file_list/SFL_CFG.ini                     ./mediatek/custom/arima89_we_s_jb2/security/sec_file_list/SFL_CFG.ini                
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/sml_auth/SML_AUTH_CFG.ini                     ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SML_AUTH_CFG.ini                
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/sml_auth/SDS_AUTH_CFG.ini                     ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SDS_AUTH_CFG.ini                
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/sml_auth/SDS_ENCODE_CFG.ini                   ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SDS_ENCODE_CFG.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/sml_auth/SML_ENCODE_KEY.ini                   ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SML_ENCODE_KEY.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/sml_auth/SML_AUTH_KEY.ini                     ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SML_AUTH_KEY.ini                
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/sml_auth/SML_ENCODE_CFG.ini                   ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SML_ENCODE_CFG.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/image_encrypt/IMG_ENCRYPT_CFG.ini             ./mediatek/custom/arima89_we_s_jb2/security/image_encrypt/IMG_ENCRYPT_CFG.ini        
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/image_encrypt/IMG_ENCRYPT_KEY.ini             ./mediatek/custom/arima89_we_s_jb2/security/image_encrypt/IMG_ENCRYPT_KEY.ini        
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/brown/recovery/SEC_VER.txt 			             ./mediatek/custom/arima89_we_s_jb2/security/recovery/SEC_VER.txt        		
		echo "[key transfer]Copied MTK key settings to the target."
		
		echo rm ./mediatek/custom/arima89_we_s_jb2/security/sec_file_list/SECRO_SFL.ini              
		echo rm ./mediatek/custom/arima89_we_s_jb2/security/sec_file_list/ANDRO_SFL.ini              
		echo echo "[key transfer]Delete done SECRO_SFL.in and ANDRO_SFL.inii."
		echo cp -f ./mediatek/external/seclib/S_ANDRO_SFL.ini.signed.mtk ./mediatek/external/seclib/S_ANDRO_SFL.ini
		echo cp -f ./mediatek/platform/mt6577/kernel/core/include/mach/sbchk_base.h.arima89_we_s_jb2 ./mediatek/platform/mt6577/kernel/core/include/mach/sbchk_base.h		
	
	elif [ "$yn" == "6" ]; then

		echo MTK_SEC_BOOT=ATTR_SBOOT_ENABLE
		echo MTK_SEC_CHIP_SUPPORT=yes
		echo MTK_SEC_MODEM_AUTH=yes
		echo MTK_SEC_MODEM_ENCODE=yes
		echo MTK_SEC_MODEM_NVRAM_ANTI_CLONE=yes		#TBD
		echo MTK_SEC_SECRO_AC_SUPPORT=yes
		echo MTK_SEC_USBDL=ATTR_SUSBDL_ENABLE

		sed -i 's/^\(MTK_SEC_BOOT\s*=\s*\).*$/\1ATTR_SBOOT_ENABLE/' 					./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_CHIP_SUPPORT\s*=\s*\).*$/\1yes/' 	./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_AUTH\s*=\s*\).*$/\1yes/' 		./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_ENCODE\s*=\s*\).*$/\1yes/'	./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_NVRAM_ANTI_CLONE\s*=\s*\).*$/\1yes/' ./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_SECRO_AC_SUPPORT\s*=\s*\).*$/\1yes/' 				./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_USBDL\s*=\s*\).*$/\1ATTR_SUSBDL_ENABLE/' 									./mediatek/config/common/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MP_KEY\s*=\s*\).*$/\1KEY_TYPE_RED/' 									./mediatek/config/common/ProjectConfig.mk

		sed -i 's/^\(MTK_SEC_BOOT\s*=\s*\).*$/\1ATTR_SBOOT_ENABLE/' 					./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_CHIP_SUPPORT\s*=\s*\).*$/\1yes/' 	./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_AUTH\s*=\s*\).*$/\1yes/' 		./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_ENCODE\s*=\s*\).*$/\1yes/'	./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MODEM_NVRAM_ANTI_CLONE\s*=\s*\).*$/\1yes/' ./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_SECRO_AC_SUPPORT\s*=\s*\).*$/\1yes/' 				./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_USBDL\s*=\s*\).*$/\1ATTR_SUSBDL_ENABLE/' 									./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk
		sed -i 's/^\(MTK_SEC_MP_KEY\s*=\s*\).*$/\1KEY_TYPE_RED/' 									./mediatek/config/arima89_we_s_jb2/ProjectConfig.mk

		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/red/image_auth/IMG_AUTH_CFG.ini		   	  ./mediatek/custom/arima89_we_s_jb2/security/image_auth/IMG_AUTH_CFG.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/red/image_auth/IMG_AUTH_KEY.ini                   ./mediatek/custom/arima89_we_s_jb2/security/image_auth/IMG_AUTH_KEY.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/red/chip_config/s/key/CHIP_TEST_KEY.ini           ./mediatek/custom/arima89_we_s_jb2/security/chip_config/s/key/CHIP_TEST_KEY.ini      
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/red/chip_config/s/cfg/SECURE_JTAG_CONFIG.ini      ./mediatek/custom/arima89_we_s_jb2/security/chip_config/s/cfg/SECURE_JTAG_CONFIG.ini 
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/red/chip_config/s/cfg/CHIP_CONFIG.ini             ./mediatek/custom/arima89_we_s_jb2/security/chip_config/s/cfg/CHIP_CONFIG.ini        
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/red/sec_file_list/SFL_CFG.ini                     ./mediatek/custom/arima89_we_s_jb2/security/sec_file_list/SFL_CFG.ini                
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/red/sml_auth/SML_AUTH_CFG.ini                     ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SML_AUTH_CFG.ini                
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/red/sml_auth/SDS_AUTH_CFG.ini                     ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SDS_AUTH_CFG.ini                
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/red/sml_auth/SDS_ENCODE_CFG.ini                   ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SDS_ENCODE_CFG.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/red/sml_auth/SML_ENCODE_KEY.ini                   ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SML_ENCODE_KEY.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/red/sml_auth/SML_AUTH_KEY.ini                     ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SML_AUTH_KEY.ini                
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/red/sml_auth/SML_ENCODE_CFG.ini                   ./mediatek/custom/arima89_we_s_jb2/security/sml_auth/SML_ENCODE_CFG.ini              
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/red/image_encrypt/IMG_ENCRYPT_CFG.ini             ./mediatek/custom/arima89_we_s_jb2/security/image_encrypt/IMG_ENCRYPT_CFG.ini        
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/red/image_encrypt/IMG_ENCRYPT_KEY.ini             ./mediatek/custom/arima89_we_s_jb2/security/image_encrypt/IMG_ENCRYPT_KEY.ini        
		cp -f ./mediatek/custom/arima89_we_s_jb2/security/keystore/red/recovery/SEC_VER.txt 		            		 ./mediatek/custom/arima89_we_s_jb2/security/recovery/SEC_VER.txt    		
		
		echo cp -f ./mediatek/secutool/signedmd/arima89_we_s_jb2_hspa/modem.img             ./mediatek/secutool/signedmd/arima89_we_s_jb2_hspa/modem.img
		echo cp -f ./mediatek/secutool/signedmd/arima89_we_s_jb2_hspa/S_ANDRO_SFL.ini 	 ./mediatek/source/external/seclib/S_ANDRO_SFL.ini
		
		echo rm ./mediatek/custom/arima89_we_s_jb2/security/sec_file_list/SECRO_SFL.ini              
		echo rm ./mediatek/custom/arima89_we_s_jb2/security/sec_file_list/ANDRO_SFL.ini              
		echo echo "[key transfer]Delete done SECRO_SFL.in and ANDRO_SFL.inii."
		
		echo cp -f ./mediatek/external/seclib/S_ANDRO_SFL.ini.signed.red ./mediatek/external/seclib/S_ANDRO_SFL.ini
		
		echo cp -f ./mediatek/platform/mt6577/kernel/core/include/mach/sbchk_base.h.arima89_we_s_jb2 ./mediatek/platform/mt6577/kernel/core/include/mach/sbchk_base.h

	fi
