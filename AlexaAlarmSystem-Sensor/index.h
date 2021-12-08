const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>

<meta HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">

<style>
h1 {
  font-size: 1.1em;
  text-align: center; 
  vertical-align: middle; 
  margin:0 auto;
}

p {
  font-size: 1.1em;
  text-align: center; 
  vertical-align: middle; 
  margin:0 auto;
}

table {
  font-size: 1.1em;
  text-align: left; 
  vertical-align: middle; 
  margin:0 auto;
}

.button {
  font-size: 18px;;
  text-align: center; 
}

</style>

<title>Alarm System</title>
<hr>
<h1>Alexa Alarm System</h1>
<hr>

</head>

<body style="font-family: verdana,sans-serif" BGCOLOR="#819FF7">
  
  <p><img id="stream" src=""></p>

  <table>
    <tr><td style="text-align:right;">Alarm sensor:</td><td style="color:white; width:130px"><span id='psa'></span></td></tr>
    <tr><td style="text-align:right;">Alarm state:</td><td style="color:white;"><span id='as'></span></td></tr>
    <tr><td style="text-align:right;">eCO2:</td><td style="color:white;"><span id='eCO2'></span></td></tr>
  </table>
  <p><meter style="width:300px;text-align:middle" id="eCO2-meter" value="0" min="0" low="400" high="2000" max="4000"></meter></p>
  <hr>
  
  <p>
  <input type="button" class="button" value=" On " onclick="button_clickedOn()"> 
  <input type="button" class="button" value="Off" onclick="button_clickedOff()"> 
  </p>
  
  <hr>
    
  <script>
    
    document.addEventListener("DOMContentLoaded", function(event) {
      const WS_URL = "ws://" + window.location.hostname + ":91";
      const ws = new WebSocket(WS_URL);
    
      const view = document.getElementById("stream");
        
      ws.onopen = () => {
        console.log("Connected to ${WS_URL}");
      };
      
      ws.onmessage = message => {
        if (message.data instanceof Blob) {
          var urlObject = URL.createObjectURL(message.data);
          view.src = urlObject;
        }
      };    
    });

    requestData(); // get intial data straight away 
    
    function button_clickedOn() { 
      var xhr = new XMLHttpRequest();
      xhr.open('GET', 'on', true);
      xhr.send();
      requestData();
    }
  
    function button_clickedOff() { 
      var xhr = new XMLHttpRequest();
      xhr.open('GET', 'off', true);
      xhr.send();
      requestData();
    }

    
    // request data updates every 1000 milliseconds
    setInterval(requestData, 1000);
    
    function requestData() {

      var xhr = new XMLHttpRequest();
      
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {

          if (this.responseText) { // if the returned data is not null, update the values

            var data = JSON.parse(this.responseText);

            document.getElementById("as").innerText = data.as;
            document.getElementById("psa").innerText = data.psa;
            document.getElementById("eCO2").innerText = data.eCO2;
            document.getElementById("eCO2-meter").value = data.eCO2;                                                

          } 
        } 
      }
      xhr.open('GET', 'get_data', true);
      xhr.send();
    }
   
     
  </script>

</body>

</html>

)=====";
