var site_url = "http://pebtides-time.herokuapp.com/";
var lat = 0;
var lon = 0;
var token = 'not_set';
var timeline = 0;

var locationOptions = {
  enableHighAccuracy: false, 
  maximumAge: 0, 
  timeout: 1000
};


function send_pebble_message(message) {
  Pebble.sendAppMessage(message,
      function(e) {
        console.log('Send successful.');
      },
      function(e) {
        console.log('Send failed!');
      }
    );
}

function getInt32Bytes( x ) {
    var bytes = [];
    for (var i = 0; i < 4; i++){
      bytes[i] = x & (255);
      x = x>>8;
    }
    return bytes;
}


function send_data_to_pebble(response){

    console.log('data from server:');
    console.log(JSON.stringify(response));

  var times = [];
  var heights = [];
  var events = [];
  var unit = '';
  for (var tide_event_index in response.tide_data){
    //process the tide data into data
    var tide_event = response.tide_data[tide_event_index];
    console.log(JSON.stringify(tide_event));

    times = times.concat(getInt32Bytes(tide_event.local_time));
    heights = heights.concat(getInt32Bytes(tide_event.height * 100));

    unit = tide_event.unit;
    if(tide_event.event == 'High Tide'){
      events.push(1);
    }
    else{
      events.push(0);
    }
  }

  var message = { 'NAME' : response.name,
                  'UNIT' : unit,
                  'N_EVENTS' : response.tide_data.length,
                  'TIMES' : times,
                  'HEIGHTS' : heights,
                  'EVENTS' : events};

  console.log('pebble message is:');
  console.log(JSON.stringify(message));

  send_pebble_message(message);

}

function get_data_for_user(){

    console.log('My token is ' + token);

    var request = new XMLHttpRequest();

    console.log('sending request to: ' + site_url+'users?token='+token);
    request.open('GET', site_url+'users?token='+token, true);

    request.onload = function() {
      if (request.status >= 200 && request.status < 400) {
        // Success!
        console.log('Data recieved from server successfully.');
        var tide_data = JSON.parse(request.responseText);
        send_data_to_pebble(tide_data);
      }
      else {
        // We reached our target server, but it returned an error
        console.log('Server returned an error');
      }
    };
    request.onerror = function() {
      // There was a connection error of some sort
      console.log('Could not reach server.');
    };
    request.send();
}

Pebble.addEventListener("ready",
  
    function(e) {
        console.log("Hello world! - Sent from your javascript application.");

        //request current position
        navigator.geolocation.getCurrentPosition(function (pos){
          lat = pos.coords.latitude;
          lon = pos.coords.longitude;
        }, function (err){
            console.log('location error (' + err.code + '): ' + err.message);
        }, locationOptions);


        if(Pebble.getTimelineToken){
            // if the timeline token is available e.g. using a Pebble Time watch
            Pebble.getTimelineToken(
                function (timeline_token) {
                    console.log('Timeline token obtained.');
                    token = timeline_token;
                    timeline = 1;
                },
                function (error) { 
                    console.log('Error getting timeline token: ' + error);
                    token = Pebble.getAccountToken()
                }
            );
        }
        else{
            console.log('Timeline token is not available for this watch');
            token = Pebble.getAccountToken()
        }
      get_data_for_user();
    }
);



Pebble.addEventListener('showConfiguration', function(e) {

        console.log('showing configuration page.');

        var url;

        if(lat !== 0 && lon !== 0) {
          url = site_url+'configure?token='+token+'&timeline='+timeline+'&lat='+lat+'&lon='+lon;
        }
        else {
          url = site_url+'configure?token='+token+'&timeline='+timeline;
        }

        console.log(url);
        Pebble.openURL(url);
        
});


Pebble.addEventListener('webviewclosed',
  function(e) {
    console.log('Configuration window returned: ' + e.response);
  }
);
