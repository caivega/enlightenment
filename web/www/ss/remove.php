<?php
function get_ip()
{
  if (getenv("REMOTE_ADDR")) $ip = getenv("REMOTE_ADDR");
  else $ip = "UNKNOWN";
  return $ip;
}

$img = "";
if (isset($_GET['image'])) 
  $img = $_GET['image'];

$file = "/var/www/www/ss/" . $img;
$auth_file = "/var/www/www/ss/ip-" . $img;
$ignore_file = "/var/www/www/ss/ignore-" . $img;
$abuse_file = "/var/www/www/ss/abuse-" . $img;
$auth_expire = 60 * 60;

if ($img[0] == "e" && file_exists($file))
  {
    if (time() - filemtime($auth_file) < $auth_expire) 
      {
	$auth = md5($img . get_ip());

	$fh = fopen($auth_file, "r"); 
	$head = fgets($fh); 
	fclose($fh); 

	if ($auth == $head || get_ip() == "140.211.167.168")
	  {
	    rename("/var/www/www/ss/ip-" . $img, 
		   "/var/www/www/ss/kill/ip-" . $img);
	    rename("/var/www/www/ss/abuse-" . $img,
		   "/var/www/www/ss/kill/abuse-" . $img);
	    rename("/var/www/www/ss/th-" . $img,
		   "/var/www/www/ss/kill/th-" . $img);
	    rename("/var/www/www/ss/" . $img,
		   "/var/www/www/ss/kill/" . $img);
	  }
      }
  }

header("Location: http://www.enlightenment.org/ss/");
die();
?>
