Aby wgrac na nodemcu to jak zawsze. W neopixel.htm zmien tylko:
var host = "";

Aby wgdac edit.htm na board to w terminal:
for file in `ls -A1`; do curl -F "file=@$PWD/$file" 192.168.178.47/edit; done

Aby wgdac to wszystko na esp8266 to zabacz na ten film:
https://www.youtube.com/watch?v=N5MoXarCF_4
Podczas wgdywania GPIO 0 musi byc na Ground. RESET ma byc jak przycisk. Podlacz go Ground i odlacz. 
Jak wgrywasz program, to bedzie sie probowac polaczyc. Jak nie bedzie mogl, to znowu podlacz RESET na krotko go Groud i wyciagnij.
Czasami jeszcze raz jak jakos nie zadziala. 
Jak wgra juz, to odlacz GPIO 0 od ground i podlacz i odlacz RESET do groud. Wtedy wystartuje na nowo i program dziala.