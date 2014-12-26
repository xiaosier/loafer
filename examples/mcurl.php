<?php 
$array = array(
        array('url' => 'http://lazy.i.sae.sina.com.cn/t.php' , 'postdata' => array('a' => 'test', 'b' => 'test2'), 'method' => 'post'),
        array('url' => 'http://lazy.i.sae.sina.com.cn/t.php', 'method' => 'post', 'postdata' => array('v' => 2)),
        array('url' => 'http://lazy.i.sae.sina.com.cn/t.php', 'postdata' => array('a' => 5), 'method' => 'post'),
        array('url' => 'http://lazy.i.sae.sina.com.cn/t.php', 'postdata' => array('a' => 6), 'method' => 'post'),
        );
$ret = mcurl($array);
var_dump($ret);
