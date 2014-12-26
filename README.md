loafer
======

loafer is an PHP extension for lazy Programmers which support lazy functions `mcurl` and `msql`.
`mcurl` is a php function for synchronous curl requests ,base on `select` model.
`msql` is a php function for InnoDB transation.

Dependence 
===

 - c++11
 - mysqlclient
 - libcurl
 - [php-cpp][1]
 - PHP version > 5.3

Build
===
You should build php-cpp first, you may read document [here][2].Then you should install mysql develop environment.

    yum install mariadb-devel.x86_64
Then modify `Makefile` ,set your php.ini location and `php-config` path.

When all is done, you can simplely run make && make install, and copy `php.ini` contents within this repo into your php.ini file.

Examples
===
```php
<?php 
// use mcurl function
$array = array(
        array('url' => 'http://lazy.i.sae.sina.com.cn/t.php' , 'postdata' => array('a' => 'test', 'b' => 'test2'), 'method' => 'post'),
        array('url' => 'http://lazy.i.sae.sina.com.cn/t.php', 'method' => 'post', 'postdata' => array('v' => 2)),
        array('url' => 'http://lazy.i.sae.sina.com.cn/t.php', 'postdata' => array('a' => 5), 'method' => 'post'),
        array('url' => 'http://lazy.i.sae.sina.com.cn/t.php', 'postdata' => array('a' => 6), 'method' => 'post'),
        );
$ret = mcurl($array);
var_dump($ret);
```

```php
<?php 
// use msql
$exec = array(
        "INSERT INTO `lazytest`(username,email)VALUES('aa', 'aa@test.com')",
        "INSERT INTO `t`(id)VALUES(222)",
        );
$ret = msql($exec);
var_dump($ret);
```

  [1]: https://github.com/CopernicaMarketingSoftware/PHP-CPP
  [2]: https://github.com/CopernicaMarketingSoftware/PHP-CPP
  [3]: https://github.com/CopernicaMarketingSoftware/PHP-CPP
