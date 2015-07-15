--TEST--
pcntl_wifexited()
--SKIPIF--
<?php if (!extension_loaded('pcntl')) die('skip pcntl extension not available'); ?>
--FILE--
<?php
if (getenv('is_child')) {
	exit;
}
if (getenv('is_child_bad')) {
	exit(42);
}
$pid = pcntl_spawn(PHP_BINARY, NULL, array(__FILE__), array(
	'is_child' => '1'));

pcntl_waitpid($pid, $status);

var_dump(pcntl_wifexited($status));

$pid = pcntl_spawn(PHP_BINARY, NULL, array(__FILE__), array(
	'is_child_bad' => '1'));

pcntl_waitpid($pid, $status);

var_dump(pcntl_wifexited($status));

var_dump(pcntl_wifexited());
?>
--EXPECTF--
bool(true)
bool(false)

Warning: pcntl_wifexited() expects exactly 1 parameter, 0 given in %s on line %d
NULL