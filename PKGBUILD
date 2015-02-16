# Maintainer: Jesse McClure AKA "Tribly" <jmcclure [at] cns [dot] umass [dot] edu>
_gitname="slider"
pkgname="${_gitname}-git"
pkgver=4.0
pkgrel=1
pkgdesc="PDF presentation tool"
arch=('x86_64' 'i686')
url="https://github.com/TrilbyWhite/Slider"
license=('GPL3')
depends=('poppler-glib' 'libxrandr')
makedepends=('git')
source=("${_gitname}::git://github.com/TrilbyWhite/Slider.git")
sha256sums=('SKIP')

pkgver() {
	cd "${srcdir}/${_gitname}"
	echo 4.$(git rev-list --count HEAD).$(git describe --always )
}

build() {
	cd "${srcdir}/${_gitname}"
	make
}

package() {
	cd "${srcdir}/${_gitname}"
	make DESTDIR="${pkgdir}" PREFIX=/usr install
}
