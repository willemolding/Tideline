site_url = "http://131.217.33.190:5000/"


function notify_pebble_no_data(){
  var message = {'DATA_AVAILABLE' : 0,
                  'NAME' : 0,
                  'N_POINTS' : 0,
                  'HEIGHTS' : 0,
                  'TIMES' : 0,
                  'UNITS' : 0}

  Pebble.sendAppMessage(message,
    function(e) {
      console.log('Send successful.');
    },
    function(e) {
      console.log('Send failed!');
    }
  );
}

function send_data_to_pebble(data){

  var heights = []
  var times = []
  for (var tide_event in data){
    heights.push(tide_event.height)
  }

  var message = {'DATA_AVAILABLE' : 1,
                  'NAME' : 'test',
                  'N_POINTS' : data.length,
                  'UNITS' : data[0].units}

    Pebble.sendAppMessage(message,
      function(e) {
        console.log('Send successful.');
      },
      function(e) {
        console.log('Send failed!');
      }
    );
}


Pebble.addEventListener("ready",
  
    function(e) {
        console.log("Hello world! - Sent from your javascript application.");

      Pebble.getTimelineToken(
		  function (token) {
		    console.log('My timeline token is ' + token);


        var request = new XMLHttpRequest();

        console.log('sending request to: ' + site_url+'users?token='+token);
        request.open('GET', site_url+'users?token='+token, true);

        request.onload = function() {
          if (request.status >= 200 && request.status < 400) {
            // Success!
            console.log('Data recieved from server successfully.');
            var data = JSON.parse(request.responseText);
            notify_pebble_no_data()
          }
          else {
            // We reached our target server, but it returned an error
            console.log('Server returned an error');
            notify_pebble_no_data()
          }
        };
        request.onerror = function() {
          // There was a connection error of some sort
          console.log('Could not reach server.');
          notify_pebble_no_data()
        };
        request.send();
		  },

		  function (error) { 
		    console.log('Error getting timeline token: ' + error);
		  }
		);

    }
);


Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  Pebble.getTimelineToken(
      function (token) {
        console.log('showing configuration page.');
        Pebble.openURL(site_url+'configure?token='+token);
      },
      function (error) { 
        console.log('Error getting timeline token: ' + error);
      }
    );
});


Pebble.addEventListener('webviewclosed',
  function(e) {
    console.log('Configuration window returned: ' + e.response);
  }
);
