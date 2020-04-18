<script>
testRunner.setCustomTextOutput(
  'method: "<?=$_SERVER['REQUEST_METHOD'] ?>"\n' +
  'formValue: "<?= $_POST['a'] ?>"');
testRunner.notifyDone();
</script>
