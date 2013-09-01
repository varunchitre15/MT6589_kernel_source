# Common functions used in MTK build extension

empty :=
space := $(empty) $(empty)

# emit-line, <word list>, <output file>
define emit-line.mtk
    $(if $(1), \
        $(eval _result := $(subst $(space),\n,$(strip $(1)))) \
        $(shell echo -e '$(strip $(_result))' >> $(2)) \
        $(eval _result :=) \
     )
endef

# dump-words-to-file, <word list>, <output file>
define dump-words-to-file.mtk
$(strip \
    $(call emit-line.mtk,$(wordlist 1,200,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 201,400,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 401,600,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 601,800,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 801,1000,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 1001,1200,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 1201,1400,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 1401,1600,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 1601,1800,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 1801,2000,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 2001,2200,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 2201,2400,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 2401,2600,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 2601,2800,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 2801,3000,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 3001,3200,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 3201,3400,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 3401,3600,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 3601,3800,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 3801,4000,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 4001,4200,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 4201,4400,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 4401,4600,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 4601,4800,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 4801,5000,$(1)),$(2)) \
    $(call emit-line.mtk,$(wordlist 5001,5200,$(1)),$(2)) \
    $(if $(wordlist 5201,5202,$(1)),$(error Too many words ($(words $(1))))) \
 )
endef

