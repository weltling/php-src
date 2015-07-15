--TEST--
pcntl_exec()
--SKIPIF--
<?php 
if (!extension_loaded("pcntl")) print "skip"; 
?>
--FILE--
<?php
echo "ok\n";
pcntl_exec(PHP_BINARY, ['-n']);
echo "nok\n";
?>
--EXPECT--
ok
