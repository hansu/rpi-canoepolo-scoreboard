const scoreboard = (function () {
  var server = "ws://scoreboard.local:8080";
  var ws;
  var reconnectIntervall, updateIntervall;
  var connected = false;
  var colorTeamA = "255,0,0";
  var colorTeamB = "0,255,0";
  function _connect() {
    if (connected) return;
    ws = new WebSocket(server);

    ws.onopen = function () {
      connected = true;
      console.log("Connected to scoreboard");
      clearInterval(reconnectIntervall);
      updateIntervall = setInterval(function () {
        sendUpdate();
      }, 500);
      $("#scoreboardConnectionStatus").css("fill", "darkgreen");
    };

    ws.onclose = function (event) {
      connected = false;
      clearInterval(reconnectIntervall);
      clearInterval(updateIntervall);
      reconnect();
      $("#scoreboardConnectionStatus").css("fill", "darkred");
    };

    ws.onerror = function (event) {
      ws.onclose(event);

      //for testing
      //sendUpdate();
    };
    function reconnect() {
      reconnectIntervall = setInterval(function () {
        _connect();
      }, 1000);
      console.log("Trying to reconnect to scoreboard");
    }
  }

  function sendUpdate() {
    var msg = "";
    if (cockpitStorage.getMatch() != null) {
      var teamAData = {
        score: $(".matchResult .goalsA").data("value"),
        color: colorTeamA,
      };

      var teamBData = {
        score: $(".matchResult .goalsB").data("value"),
        color: colorTeamB,
      };

      if (cockpitStorage.getMatch().MatchStatus == matchStatus.FirstHalf) {
        msg += `ScoreL=${teamAData.score};ColorL=${teamAData.color};`;
        msg += `ScoreR=${teamBData.score};ColorR=${teamBData.color};`;
      } else {
        msg += `ScoreL=${teamBData.score};ColorL=${teamBData.color};`;
        msg += `ScoreR=${teamAData.score};ColorR=${teamAData.color};`;
      }
      msg += `Time=${cockpitClock.getClockDatails().secondsRemaining};`;
      msg += `Shotclock=${cockpitClock.getClockDatails().shotClockRemaining};`;
      if (connected) ws.send(msg);
      // for testing
      //console.log(msg)
    }
  }

  function _hexToRgbString(hex) {
    var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result
      ? parseInt(result[1], 16) +
          "," +
          parseInt(result[2], 16) +
          "," +
          parseInt(result[3], 16)
      : null;
  }

  function _init(view) {
    $(".leftSide .teamName").append(
      "<input type='color' class='colorPicker' id='changeColorTeamA' value='#ff0000'/>"
    );
    $(document).on("change", "#changeColorTeamA", function (event) {
      colorTeamA = _hexToRgbString(event.target.value);
    });
    $(".rightSide .teamName").append(
      "<input type='color' class='colorPicker' id='changeColorTeamA' value='#00ff00'/>"
    );
    $(document).on("change", "#changeColorTeamB", function (event) {
      colorTeamB = _hexToRgbString(event.target.value);
    });
    $(document).on("click", "#waitButton", function (event) {
      colorTeamA = "255,0,0";
      colorTeamB = "0,255,0";
      $("#changeColorTeamA").value = "#ff0000";
      $("#changeColorTeamB").value = "#00ff00";
    });
    $("#connectivityIndicator .container").append(
      "<div id='scoreboardConnectionStatus' class='scoreboardConnectionStatus knlTopMenu knlTopMenuSmall hidden-xs hidden-inApp' title='Connection status to scoreboard'> <svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 448 512'> <path d='M176 0c-17.7 0-32 14.3-32 32s14.3 32 32 32h16V98.4C92.3 113.8 16 200 16 304c0 114.9 93.1 208 208 208s208-93.1 208-208c0-41.8-12.3-80.7-33.5-113.2l24.1-24.1c12.5-12.5 12.5-32.8 0-45.3s-32.8-12.5-45.3 0L355.7 143c-28.1-23-62.2-38.8-99.7-44.6V64h16c17.7 0 32-14.3 32-32s-14.3-32-32-32H224 176zm72 192V320c0 13.3-10.7 24-24 24s-24-10.7-24-24V192c0-13.3 10.7-24 24-24s24 10.7 24 24z' /> </svg> </div>"
    );
  }

  const me = {
    connect: _connect,
    init: _init,
  };
  return me;
})();

scoreboard.connect();
scoreboard.init();
