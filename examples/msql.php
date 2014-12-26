<?php 
// test for msql
$exec = array(
        "INSERT INTO `lazytest`(username,email)VALUES('aa', 'aa@test.com')",
        "INSERT INTO `t`(id)VALUES(222)",
        );
$ret = msql($exec);
var_dump($ret);
