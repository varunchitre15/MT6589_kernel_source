function Usage
{
    cat<<HELP
Usage: kernel/scripts/artifacts/`basename $0` scope project
Description:
    project
        project represent the correct project name
    scope
        scope represent the chose scope,for example:oversea
Example:
   kernel/scripts/artifacts/`basename $0` oversea oppo
   kernel/scripts/artifacts/`basename $0` other generic
Attention:
    this tool need 2 arguments
    for emulator,please use generic for the project argument
HELP
}

while getopts "h" OPT;do
case $OPT in
h) Usage;exit 1 ;;
?) Usage;exit 1 ;;
esac
done
shift `expr $OPTIND - 1`
if [ $# != 2 ];then
    Usage
    exit 1
fi

arguments=(oversea other)
flag=
for arg in ${arguments};do
    if [ "$1" = "${arg}" ];then
        flag=true
        break
    fi
done
if [ -z "$flag" ];then
    echo "Error!the first argument must in \"${arguments[@]}\""
    exit 1
fi

prjConfig="../mediatek/config/$2/ProjectConfig.mk"
if [ ! -e "${prjConfig}" ]; then
    echo "Error!the argument $2 is not a correct project name!"
    echo "please enter -h for help infomation!"
    exit 1
fi

outdir=../vendor/mediatek/$2/artifacts/kernel
outdir_obj=$outdir/obj
if [[ $1 == "other" ]]; then
  mkdir -p arch/arm/mach-mt3351
  touch arch/arm/mach-mt3351/Kconfig
  touch arch/arm/mach-mt3351/Makefile
  mkdir -p drivers/actuator
  touch drivers/actuator/Kconfig
  touch drivers/actuator/Makefile
  mkdir -p drivers/meta
  touch drivers/meta/Kconfig
  touch drivers/meta/Makefile
  mkdir -p drivers/monitor
  touch drivers/monitor/Kconfig
  touch drivers/monitor/Makefile
fi 
rm -rf $outdir_obj
mkdir -p $outdir_obj

cp -f ../mediatek/platform/mt6516/kernel/core/ccci*o $outdir_obj
cp -f ../mediatek/platform/mt6516/kernel/core/prebuilt/ccci_fs_utils.o $outdir_obj
cp -f ../mediatek/platform/mt6516/kernel/core/MT6516_PM_api.o $outdir_obj
cp -f ../mediatek/platform/mt6516/kernel/core/mt6516_intr.o $outdir_obj
cp -f ../mediatek/platform/mt6516/kernel/core/MT6516_sleep.o $outdir_obj
#touch ../mediatek/platform/mt6516/kernel/core/aed.o
#cp -f ../mediatek/source/kernel/drivers/aee/aed.o $outdir_obj
if [ -e ../mediatek/source/kernel/drivers/net/mt592x/wlan/gl_sec.o ]; then
  cp -f ../mediatek/source/kernel/drivers/net/mt592x/wlan/gl_sec.o $outdir_obj
fi
if [[ $1 == "other" ]]; then
  cp -f arch/arm/mach-mt6516/core.o $outdir_obj
  cp -f arch/arm/mach-mt6516/pwm.o $outdir_obj
  cp -f arch/arm/mach-mt6516/dma.o $outdir_obj
  cp -f arch/arm/mach-mt6516/mt6516_pll.o $outdir_obj
  cp -f arch/arm/mach-mt6516/MT6516_sleep.o $outdir_obj
  cp -f arch/arm/mach-mt6516/mt6516_wdt.o $outdir_obj
  cp -f arch/arm/mach-mt6516/mt6516_timer.o $outdir_obj
  cp -f arch/arm/mach-mt6516/gpt.o $outdir_obj
  cp -f arch/arm/mach-mt6516/mt6516_IDP.o $outdir_obj
  cp -f arch/arm/mach-mt6516/mt6516_ISP.o $outdir_obj
  cp -f arch/arm/mach-mt6516/system.o $outdir_obj
  cp -f arch/arm/mach-mt6516/mt6516_busmonitor.o $outdir_obj
  cp -f arch/arm/mach-mt6516/mt6516_devs.o $outdir_obj
  cp -f drivers/char/sampletrigger.o $outdir_obj
  cp -f drivers/mmc/host/mt6516_sd.o $outdir_obj
  cp -f drivers/power/smart_battery_mt6516.o $outdir_obj
fi

for item in "$outdir_obj"/*; do
  mv ${item} ${item}.artifacts
done

cp scripts/artifacts/objects-$1.mk $outdir/objects.mk

#chmod 755 scripts/Makefile.build
#echo "include binary/Makefile" >> scripts/Makefile.build
#chmod 444 scripts/Makefile.build
