--TEST--
Bug #43344.10 (Wrong error message for undefined namespace constant)
--FILE--
<?php
echo namespace\bar."\n";
?>
--EXPECTF--
Fatal error: Uncaught exception 'EngineException' with message 'Undefined constant 'bar'' in %sbug43344_10.php:%d
Stack trace:
#0 {main}
  thrown in %sbug43344_10.php on line %d
