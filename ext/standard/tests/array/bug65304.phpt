--TEST--
Bug #65304 (Use of max int in array_sum)
--FILE--
<?php
var_dump(array_sum(array(PHP_INT_MAX, 1)));
var_dump(PHP_INT_MAX + 1);
?>
--EXPECTF--	
int(9223372036854775808)
int(9223372036854775808)