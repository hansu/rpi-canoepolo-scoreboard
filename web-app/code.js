/* WebSocket. */
var ws;
var connected = false;
var scoreL,scoreR, minutes, seconds, shotclock;
function scoll_to_bottom() {
    var logTa = document.getElementById("taLog")
    logTa.scrollTop = logTa.scrollHeight;
}

/* Establish connection. */
function doConnect(addr) {
    /* Message to be sent. */
    var msg;

    /* Do connection. */
    ws = new WebSocket(addr);

    /* Register events. */
    ws.onopen = function () {
        connected = true;
        document.getElementById("btConn").value = "Disconnect!";
        document.getElementById("taLog").value += ("Connection opened\n");
        scoll_to_bottom();
    };

    /* Deals with messages. */
    ws.onmessage = function (evt) {
        document.getElementById("taLog").value += ("Recv: " + evt.data + "\n");
        scoll_to_bottom();
        update(evt.data)
    };

    /* Close events. */
    ws.onclose = function (event) {
        document.getElementById("btConn").value = "Connect!";
        document.getElementById("taLog").value +=
            ("Connection closed: wasClean: " + event.wasClean + ", evCode: "
                + event.code + "\n");
        scoll_to_bottom();
        connected = false;
    };
}
function update(data){
    console.log(data)
    var obj = JSON.parse(data);
console.log(scoreL);
scoreL.innerHTML = obj.score[0];
scoreR.innerHTML = obj.score[1];
minutes.innerHTML = obj.time[0];
seconds.innerHTML = obj.time[1]<10?"0"+obj.time[1]:obj.time[1];
shotclock.innerHTML = obj.shotclock;

obj.shotclock<10?shotclock.style.color="red":shotclock.style.color="green";



}

document.addEventListener("DOMContentLoaded", function (event) {
    /* Connect buttom. */
    document.getElementById("btConn").onclick = function () {
        if (connected == false) {
            var txt = document.getElementById("txtServer").value;
            doConnect(txt);
        }

        else {
            ws.close();
            connected = false;
            document.getElementById("btConn").value = "Connect!";
        }
    };

    /* Input text message. */
    document.getElementById("txtMsg").addEventListener('keyup', function (e) {
        var key = e.which || e.keyCode;
        if (key == 13)
            document.getElementById("btMsg").click();
    });

    /* Send message. */
    document.getElementById("btMsg").onclick = function () {
        if (connected == true) {
            var txt = document.getElementById("txtMsg");
            var log = document.getElementById("taLog");

            ws.send(txt.value);
            log.value += ("Send: " + txt.value + "\n");
            txt.value = "";
            scoll_to_bottom();
        }
    };
    scoreL = document.getElementById("scoreL");
    scoreR = document.getElementById("scoreR");
    minutes = document.getElementById("minutes");
    seconds = document.getElementById("seconds");
    shotclock = document.getElementById("shotclock");


});