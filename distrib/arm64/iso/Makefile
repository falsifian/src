#	$OpenBSD: Makefile,v 1.1 2021/04/10 21:02:36 deraadt Exp $

FS=		install${OSrev}.img
FSSIZE=		1032192
FSDISKTYPE=	install

MOUNT_POINT=	/mnt

RELXDIR?=	/home/relx-${MACHINE}
RELDIR?=	/home/rel-${MACHINE}

BASE=		${RELDIR}/base${OSrev}.tgz ${RELDIR}/comp${OSrev}.tgz \
		${RELDIR}/game${OSrev}.tgz ${RELDIR}/man${OSrev}.tgz \
		${RELDIR}/bsd ${RELDIR}/bsd.mp ${RELDIR}/bsd.rd \
		${RELDIR}/INSTALL.${MACHINE}
XBASE=		${RELXDIR}/xbase${OSrev}.tgz ${RELXDIR}/xfont${OSrev}.tgz \
		${RELXDIR}/xshare${OSrev}.tgz ${RELXDIR}/xserv${OSrev}.tgz

MSDOSSTART=	32768
MSDOSSIZE=	8192
FFSSTART!=	expr ${MSDOSSTART} + ${MSDOSSIZE}

NEWFS_ARGS_msdos=-L boot -c1 -F16
MOUNT_ARGS_msdos=-o-l

PDTB=		/usr/local/share/dtb/arm64
PUBOOT=		/usr/local/share/u-boot
PRPI=		/usr/local/share/raspberrypi-firmware/boot

PIFILES=\
	bootcode.bin \
	start.elf \
	fixup.dat \
	bcm2710-rpi-3-b.dtb \
	bcm2710-rpi-3-b-plus.dtb \
	bcm2710-rpi-cm3.dtb

PIDTBO=\
	disable-bt.dtbo

all: ${FS}

${FS}: ${BASE} ${XBASE}
	dd if=/dev/zero of=${FS} bs=512 count=${FSSIZE}
	vnconfig -v ${FS} > vnd
	echo 'u\ne 0\nc\nn\n${MSDOSSTART}\n${MSDOSSIZE}\ne 3\nA6\nn\n${FFSSTART}\n*\nf 0\nw\nq\n' \
	    | fdisk -e `cat vnd` >/dev/null
	echo 'a a\n\n\n\nw\nq\n' | disklabel -E `cat vnd` >/dev/null
	newfs -t msdos ${NEWFS_ARGS_msdos} /dev/r`cat vnd`i
	mount ${MOUNT_ARGS_msdos} /dev/`cat vnd`i ${MOUNT_POINT}
.for FILE in ${PIFILES}
	cp ${PRPI}/${FILE} ${MOUNT_POINT}/
.endfor
	mkdir -p ${MOUNT_POINT}/overlays
.for FILE in ${PIDTBO}
	cp ${PRPI}/overlays/${FILE} ${MOUNT_POINT}/overlays/
.endfor
	cp ${PUBOOT}/rpi_3/u-boot.bin ${MOUNT_POINT}/
	mkdir -p ${MOUNT_POINT}/efi/boot
	cp /usr/mdec/BOOTAA64.EFI ${MOUNT_POINT}/efi/boot/bootaa64.efi
	echo bootaa64.efi > ${MOUNT_POINT}/efi/boot/startup.nsh
	echo 'arm_64bit=1\nenable_uart=1\ndtoverlay=disable-bt\nkernel=u-boot.bin' > ${MOUNT_POINT}/config.txt
	dd if=${PUBOOT}/pine64_plus/u-boot-sunxi-with-spl.bin \
	    of=/dev/r`cat vnd`c bs=1024 seek=8 conv=sync
	umount ${MOUNT_POINT}

	newfs -O 1 -m 0 -o space -i 524288 -c ${FSSIZE} /dev/r`cat vnd`a
	mount /dev/`cat vnd`a ${MOUNT_POINT}
	mkdir -p ${MOUNT_POINT}/${OSREV}/${MACHINE}
	cp -p ${BASE} ${MOUNT_POINT}/${OSREV}/${MACHINE}
	cp -p ${XBASE} ${MOUNT_POINT}/${OSREV}/${MACHINE}
	ln ${MOUNT_POINT}/${OSREV}/${MACHINE}/bsd.rd ${MOUNT_POINT}/bsd.rd
	ln ${MOUNT_POINT}/${OSREV}/${MACHINE}/bsd.rd ${MOUNT_POINT}/bsd

	cat ${RELDIR}/SHA256 ${RELXDIR}/SHA256 > \
	    ${MOUNT_POINT}/${OSREV}/${MACHINE}/SHA256
	# XXX no SHA256.sig
	df -i ${MOUNT_POINT}
	umount ${MOUNT_POINT}
	vnconfig -u `cat vnd`
	rm -f vnd

install:
	cp ${CDROM} ${FS} ${RELDIR}/

clean cleandir:
	rm -f ${CDROM} ${FS}
	rm -rf cd-dir

.include <bsd.obj.mk>
