NGINX_VERSION=1.7.5
PCRE_VERSION=8.35
HEADERS_MORE_VERSION=0.23

nginx_tarball_url=http://nginx.org/download/nginx-${NGINX_VERSION}.tar.gz
pcre_tarball_url=http://garr.dl.sourceforge.net/project/pcre/pcre/${PCRE_VERSION}/pcre-${PCRE_VERSION}.tar.bz2
headers_more_nginx_module_url=https://github.com/agentzh/headers-more-nginx-module/archive/v${HEADERS_MORE_VERSION}.tar.gz

echo "Start script"
cur_dir=`pwd`
prefix="/usr/local"
if ( $1 == 'heroku'); then
	( cd /tmp ; python -m SimpleHTTPServer $PORT & )
	prefix="/tmp/nginx"
fi

temp_dir=$(mktemp -d /tmp/nginx.XXXXXXXXXX)
echo "Move module ngx_http_libvlc_module to /${temp_dir}"
cp -r ngx_http_libvlc_module /${temp_dir}/.

cd $temp_dir
echo "Temp dir: $temp_dir"

echo "Downloading $nginx_tarball_url"
curl -L $nginx_tarball_url | tar xzv

echo "Downloading $pcre_tarball_url"
(cd nginx-${NGINX_VERSION} && curl -L $pcre_tarball_url | tar xvj )

echo "Downloading $headers_more_nginx_module_url"
(cd nginx-${NGINX_VERSION} && curl -L $headers_more_nginx_module_url | tar xvz )

echo "Compile nginx with http_libvlc module"
(	
	cd nginx-${NGINX_VERSION}
	./configure \
		--with-pcre=pcre-${PCRE_VERSION} \
		--prefix=$prefix \
		--add-module=/${temp_dir}/nginx-${NGINX_VERSION}/headers-more-nginx-module-${HEADERS_MORE_VERSION} \
		--add-module=/${temp_dir}/ngx_http_libvlc_module 
	sudo make install
)

if ( $1 != 'heroku'); then
	sudo apt-get -y install ruby
	erb $cur_dir/config/nginx.conf.erb > /usr/local/conf/nginx.conf
fi

while true
do
	sleep 10
	echo "."
done
