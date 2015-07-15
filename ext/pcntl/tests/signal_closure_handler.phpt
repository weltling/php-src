--TEST--
Closures as a signal handler
--SKIPIF--
<?php
	if (!extension_loaded("pcntl")) print "skip"; 
	if (!function_exists("pcntl_signal")) print "skip pcntl_signal() not available";
?>
--FILE--
<?php
declare (ticks = 1);

pcntl_signal(SIGTERM, function ($signo) { echo "Signal handler called!\n"; });

echo "Start!\n";
pcntl_raise(SIGTERM);
$i = 0; // dummy
echo "Done!\n";

?>
--EXPECTF--
Start!
Signal handler called!
Done!
