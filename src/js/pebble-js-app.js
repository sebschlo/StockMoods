var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

var selectedSymbol = "YHOO";
var options = {};

function quoteStock(symbol) {
  // Construct URL
  var url = "http://dev.markitondemand.com/Api/v2/Quote/json?symbol=" + symbol;


  // Send request to Markit Stocks API
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with a quote for a specific stock
      var json = JSON.parse(responseText);
      var sign = -1;
      // Log the change
      console.log("Percentage change is: "+json.ChangePercent);

      if (json.ChangePercent > 0.5) {
        sign = 0;
      } else if(json.ChangePercent > 0) {
        sign = 1;
      } else {
        sign = 2;
      }

      // Assemble dictionary using our keys
      var dictionary = {
        "KEY_SYMBOL": json.Symbol,
        "KEY_CHANGE": json.ChangePercent.toFixed(2).toString(),
        "KEY_SIGN": sign
      };
 
      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Stock info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending stock info to Pebble!");
        }
      );
    }      
  );
}

// Listen for when the watchface is opened and fetch data
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");

    // Get the initial stock data   
    quoteStock(selectedSymbol);
  }
);

// Listen for when an AppMessage is received, meaning that a refresh 
// was requested
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    quoteStock(selectedSymbol);
  }                     
);

// Listen for the configuration page command
Pebble.addEventListener("showConfiguration", function() {
  console.log("showing configuration");
  Pebble.openURL('http://pebble-demo-embedded.s3-website-us-east-1.amazonaws.com/index.html');
});

// Listen for the configuration being done
Pebble.addEventListener("webviewclosed", function(e) {
  console.log("configuration closed");

  if (e.response.charAt(0) == "{" && e.response.slice(-1) == "}" && e.response.length > 2) {
    options = JSON.parse(decodeURIComponent(e.response));
    console.log("Options = " + JSON.stringify(options));
    selectedSymbol = options.symbol;
    quoteStock(selectedSymbol);
  } else {
    console.log("cancelled");
  }
});
