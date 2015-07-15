--TEST--
pcntl_exec() with environment variables
--SKIPIF--
<?php

if (!extension_loaded("pcntl")) print "skip"; 

?>
--FILE--
<?php
if (getenv("PCNTL_EXEC_TEST_IS_CHILD")) {
	var_dump((string)getenv("FOO"));
	exit;
}
echo "ok\n";
pcntl_exec(PHP_BINARY, array('-n', __FILE__), array(
	"PCNTL_EXEC_TEST_IS_CHILD" => "1", 
	"FOO" => "BAR")
);

echo "nok\n";
?>
--EXPECT--
ok
string(3) "BAR"
