Pebble.addEventListener("ready",
  
    function(e) {
        console.log("Hello world! - Sent from your javascript application.");

      Pebble.getTimelineToken(
		  function (token) {
		    console.log('My timeline token is ' + token);

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
        Pebble.openURL('http://pebtides-time.herokuapp.com/configure?token='+token);
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
