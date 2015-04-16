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
  Pebble.openURL('https://my-website.com/config-page.html');
});


Pebble.addEventListener('webviewclosed',
  function(e) {
    console.log('Configuration window returned: ' + e.response);
  }
);
