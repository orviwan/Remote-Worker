Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://9ecdf39b.ngrok.io/index.html';
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var data = JSON.parse(decodeURIComponent(e.response));
  //console.log('Configuration page returned: ' + JSON.stringify(configData));
  var dict = {};
  dict['BACKGROUND_KEY'] = parseInt(data['Background'], 16);
  dict['LOCALTIME_KEY'] = parseInt(data['LocalTime'], 16);
  dict['REMOTETIME_KEY'] = parseInt(data['RemoteTime'], 16);
  dict['DAY_KEY'] = parseInt(data['Day'], 16);
  dict['DATE_KEY'] = parseInt(data['Date'], 16);
  dict['OFFSET_KEY'] = parseInt(data['Offset']) * 60 * 60;
  dict['LABEL_KEY'] = data['Label'];

  // Send to watchapp
  Pebble.sendAppMessage(dict, function() {
    console.log('Send successful: ' + JSON.stringify(dict));
  }, function() {
    console.log('Send failed!');
  });
});
