# New ports collection makefile for:	cdparanoia
# Date created:				5 Jan 2003
# Whom:			Simon 'corecode' Schubert <corecode@corecode.ath.cx>
#
# $FreeBSD: ports/audio/cdparanoia/Makefile,v 1.18 2010/12/21 16:59:39 danfe Exp $
#

PORTNAME=	cdparanoia
PORTVERSION=	3.10.2
PORTREVISION=	0
CATEGORIES=	audio sysutils
MASTER_SITES=	http://downloads.xiph.org/releases/cdparanoia/
DISTNAME=	${PORTNAME}-${PORTVERSION:C/^3\./III-/}
EXTRACT_SUFX=	.src.tgz

MAINTAINER=	danfe@FreeBSD.org
COMMENT=	A CDDA extraction tool (also known as ripper)

GNU_CONFIGURE=	yes
USE_GMAKE=	yes
USE_LDCONFIG=	yes

MAN1=		cdparanoia.1

post-patch:
	@${REINPLACE_CMD} -e 's/\(^ *default:\)/\1break;/' ${WRKSRC}/interface/utils.h

.include <bsd.port.mk>
