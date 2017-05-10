#!/bin/bash
maxnconn=3

if [ $1 ]; then
	maxnconn=$1
fi

#time ./web $maxnconn www.nipic.com / /show/8817711.html /show/8817712.html /show/8817713.html /show/8817714.html

time ./web $maxnconn www.zcool.com.cn / \
	/img.html#src=http://img.zcool.cn/haha.jpg\
	/img.html#src=http://img.zcool.cn/community/015db259109624b5b3086ed4fa7b75.jpg\
	/img.html#src=http://img.zcool.cn/community/0101fa5911bd70b5b3086ed42f956a.jpg\
	/img.html#src=http://img.zcool.cn/community/0187e55911bd6db5b3086ed4ccbfb8.jpg\
	/img.html#src=http://img.zcool.cn/community/019aab59116ee2a801216a3e4bdc28.jpg\
	/img.html#src=http://img.zcool.cn/community/01cf285911b3b1b5b3086ed431ed7b.jpg\
	/img.html#src=http://img.zcool.cn/community/01154759115ae6b5b3086ed4e7282f.jpg\
	/img.html#src=http://img.zcool.cn/community/01095f59115aebb5b3086ed40f0831.jpg\
	/img.html#src=http://img.zcool.cn/community/01864a59115b1bb5b3086ed49cbd68.jpg\
	/img.html#src=http://img.zcool.cn/community/01091f59115b41b5b3086ed47f40a3.jpg\
	/img.html#src=http://img.zcool.cn/community/0125c959115b2ea801216a3e82f77c.jpg\
	/img.html#src=http://img.zcool.cn/community/01a52959115b36b5b3086ed450685c.jpg\
	/img.html#src=http://img.zcool.cn/community/01eed559115b3ba801216a3e8c0c54.jpg\
	/img.html#src=http://img.zcool.cn/community/017d9459115b7ea801216a3e3a14fc.jpg\
	/img.html#src=http://img.zcool.cn/community/01f3e159115b8ab5b3086ed439dee9.jpg\
	/img.html#src=http://img.zcool.cn/community/01d76a59115c39a801216a3eb15cbb.jpg\
	/img.html#src=http://img.zcool.cn/community/017e2759115c43a801216a3e45ebdc.jpg\
	/img.html#src=http://img.zcool.cn/community/01368259115c36b5b3086ed4b987da.jpg\
	/img.html#src=http://img.zcool.cn/community/012d8559115c3fb5b3086ed4649e92.jpg\
	/img.html#src=http://img.zcool.cn/community/01728659115c41b5b3086ed47052a8.jpg\
	/img.html#src=http://img.zcool.cn/community/01a0685911296bb5b3086ed41b6ae5.jpg\
	/img.html#src=http://img.zcool.cn/community/011e8959118510b5b3086ed476a290.jpg\
	/img.html#src=http://img.zcool.cn/community/01a7b959118531a801216a3e317975.jpg\
	/img.html#src=http://img.zcool.cn/community/01590d59117739b5b3086ed41c396e.jpg\
	/img.html#src=http://img.zcool.cn/community/011cb359117722a801216a3e625e83.jpg\
	/img.html#src=http://img.zcool.cn/community/0181055911771ca801216a3e531e6f.jpg\
	/img.html#src=http://img.zcool.cn/community/01fdd959117703b5b3086ed453ea79.jpg\
	/img.html#src=http://img.zcool.cn/community/01f34b5911502db5b3086ed4c840a3.jpg\
	/img.html#src=http://img.zcool.cn/community/01520b59115028b5b3086ed48692c7.jpg\
	/img.html#src=http://img.zcool.cn/community/016c8859115028a801216a3e9e3d60.jpg\
	/img.html#src=http://img.zcool.cn/community/013dc459115024b5b3086ed4b65d27.jpg\
	/img.html#src=http://img.zcool.cn/community/01712559115019a801216a3ebb538a.jpg\
	/img.html#src=http://img.zcool.cn/community/01131659115018b5b3086ed46a5e8c.jpg\
	>/dev/null	

