# Maintainer: Jesse McClure AKA "Trilby" <jmcclure [at] cns [dot] umass [dot] edu>
pkgname=slider-git
pkgver=20120830
pkgrel=1
pkgdesc="PDF slideshow / presentation tool with xrandr support and presenter mode"
url="http://github.com/TrilbyWhite/Slider.git"
arch=('any')
license=('GPLv3')
depends=('poppler-glib' 'xorg-xrandr')
makedepends=('git')
_gitroot="git://github.com/TrilbyWhite/Slider.git"
_gitname="Slider"

build() {
    cd "$srcdir"
    msg "Connecting to GIT server...."
    if [ -d $_gitname ] ; then
        cd $_gitname && git pull origin
        msg "The local files are updated."
    else
        git clone $_gitroot $_gitname
    fi
    msg "GIT checkout done or server timeout"
    msg "Starting make..."
    rm -rf "$srcdir/$_gitname-build"
    git clone "$srcdir/$_gitname" "$srcdir/$_gitname-build"
    cd "$srcdir/$_gitname-build"
	make
}

package() {
	cd "$srcdir/$_gitname-build"
	make PREFIX=/usr DESTDIR=$pkgdir install
}

