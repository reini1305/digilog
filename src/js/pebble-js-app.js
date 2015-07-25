
// We use the fake "PBL" symbol as default
var defaultBackOne = "ffaaff";
var defaultBackTwo = "005555";
var defaultNumberOne = "0000aa";
var defaultNumberTwo = "ffff55";
var BackOne;
var BackTwo;
var NumberOne;
var NumberTwo;

function sendConfig(){
  var msg = {};
  msg.numberOne = parseInt("0x"+NumberOne);
  msg.numberTwo = parseInt("0x"+NumberTwo);
  msg.backOne = parseInt("0x"+BackOne);
  msg.backTwo = parseInt("0x"+BackTwo);
  console.log("Sending new configuration");
  console.log(msg.numberOne);
  Pebble.sendAppMessage(msg);
}

// Set callback for the app ready event
Pebble.addEventListener("ready",
                        function(e) {
                          //console.log("connect! " + e.ready);
                          //console.log(e.type);
                        BackOne = localStorage.getItem('BackOne');
                        BackTwo = localStorage.getItem('BackTwo');
                        NumberOne = localStorage.getItem('NumberOne');
                        NumberTwo = localStorage.getItem('NumberTwo');
                        if (!BackOne) {
                            BackOne = defaultBackOne;
                            BackTwo = defaultBackTwo;
                            NumberOne = defaultNumberOne;
                            NumberTwo = defaultNumberTwo;
                          }
                        console.log("sent ready signal");
                          //generateCode();
                        });

// Set callback for appmessage events
Pebble.addEventListener("appmessage",
                        function(e) {
                          console.log("message");
                          if (e.payload.fetch) {
                            sendConfig();
                          }
                        });

Pebble.addEventListener('showConfiguration', function(e) {
                        var uri = 'http://reini1305.github.io/configuration/configuration_digilog.html?' +
                        'background_one=' + encodeURIComponent(BackOne)+'&background_two=' + encodeURIComponent(BackTwo)+
                        '&number_one=' + encodeURIComponent(NumberOne)+'&number_two=' + encodeURIComponent(NumberTwo);
                        console.log('showing configuration at uri: ' + uri);
                        Pebble.openURL(uri);
                        });


Pebble.addEventListener('webviewclosed', function(e) {
                        console.log('configuration closed');
                        if (e.response) {
                        var options = JSON.parse(decodeURIComponent(unescape(e.response)));
                        console.log('options received from configuration: ' + JSON.stringify(options));
                        NumberOne = options['number_one'];
                        NumberTwo = options['number_two'];
                        BackOne = options['background_one'];
                        BackTwo = options['background_two'];
                        localStorage.setItem('NumberOne',NumberOne);
                        localStorage.setItem('NumberTwo',NumberTwo);
                        localStorage.setItem('BackOne',BackOne);
                        localStorage.setItem('BackTwo',BackTwo);
                        sendConfig();
                        
                        } else {
                        console.log('no options received');
                        }
                        });
