var path = window.location.pathname;
var page = path.split("/").pop();
console.log( path );
console.log(page);
if (path == "/"){
    console.log("on index");
}

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onload);

function onload(event) {
        initWebSocket();
}

function getValues(){
    websocket.send("getData");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    if (path == "/"){
        getValues();
    }
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function handleSubmit(event) {
    event.preventDefault();

    const data = new FormData(event.target);

    const value = Object.fromEntries(data.entries());
    if(document.getElementById("screenrotated").checked){
        value.screenrotated = true
    }
    else {
        value.screenrotated = false
    }
    if(document.getElementById("flashing").checked){
        value.flashing = true
    }
    else {
        value.flashing = false
    }
    if(document.getElementById("mqttenabled").checked){
        value.mqttenabled = true
    }
    else {
        value.mqttenabled = false
    }
    if(document.getElementById("mqttretain").checked){
        value.mqttretain = true
    }
    else {
        value.mqttretain = false
    }
    
    
    console.log(value)
    delete value.rotationCheckBox
    delete value.ledsCheckBox
    const JSONstring = JSON.stringify(value);
    

    console.log( "Reboot!!" );
    websocket.send("rb"+JSONstring);

    setTimeout(function(){
        window.location.reload();
     }, 5000);
}

if (path != "/"){
    const form = document.querySelector('.setup');
    form.addEventListener('submit', handleSubmit);
}

function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    for (var i = 0; i < keys.length; i++){
        var key = keys[i];
        if (path == "/"){
            if(key == "flashing"){
                document.getElementById("Leds-toggle").checked = myObj[key];
            }
            else if(document.getElementById(key)){
                document.getElementById(key).innerHTML = myObj[key]; 
            }
        }
        else {
            if(key == "screenrotated"){
                document.getElementById("screenrotated").checked = myObj[key];
            }
            else if(key == "flashing"){
                document.getElementById("flashing").checked = myObj[key];
            }
            else if(key == "mqttenabled"){
                document.getElementById("mqttenabled").checked = myObj[key];
            }
            else if(key == "mqttretain"){
                document.getElementById("mqttretain").checked = myObj[key];
            }
            else{
                if(document.getElementById(key)){
                    document.getElementById(key).value = myObj[key]; 
                }
            }
        }
    }
}

function ledsToggle()
{
    if (document.getElementById('Leds-toggle').checked){
        console.log("flashing On")
        websocket.send("ld1")
    }
    else {
        console.log("Flashing Off")
        websocket.send("ld0")
    }
}

function resetValues(value) {
    console.log("Reset buttonclicked");
    websocket.send("rst"+value);
  }
