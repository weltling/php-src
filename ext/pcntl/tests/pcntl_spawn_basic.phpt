--TEST--
Test function pcntl_spawn() by calling it with its expected arguments
--SKIPIF--
<?php
	if (!extension_loaded('pcntl')) die('skip pcntl extension not available');
?>
--FILE--
<?php
if (getenv("PCNTL_SPAWN_TEST_IS_CHILD")) {
	var_dump(getmypid());
	var_dump(getenv("FOO"));
	exit;
}
$pid = pcntl_spawn(PHP_BINARY, NULL, array(__FILE__), array(
	"PCNTL_SPAWN_TEST_IS_CHILD" => "1", 
	"FOO" => "BAR"));

pcntl_waitpid($pid, $status, null);
var_dump($pid);
?>
--EXPECTF--
int(%d)
