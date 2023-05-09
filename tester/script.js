//tested on Safari,chrome 
//No Script
//total time: 2.4 hours 
//You Can Find Me On Instagram: @akshaycodes 
//if you have questions feel free to ask
console.clear()
let map = new Map();
let keys = new Map();

window.addEventListener("load", function() {
  keys = document.querySelectorAll('.keys');
});

function recordkeys(e) {

    let j = e.code.toLowerCase();
                
    let code = keys[0].getElementsByClassName("k"+j)[0];
    let key = keys[0].getElementsByClassName("k"+e.key.toLowerCase())[0];
 
 if (code) {
  code.classList.add("pressed");  
  map.set(j,"1");
  } else if (key)
  {  
	key.classList.add("pressed");
	map.set(j,"1");
  }
}

window.addEventListener('keydown', function(e)
{

	e.preventDefault();
	recordkeys(e);
});


document.oncontextmenu = function() {
  return false; 
};


let lastX;
let lastY;

document.addEventListener("mousemove", (e) => {
	
	var effDis = 30;// move at least 30 pixels
	if (lastX < e.clientX && e.clientX - lastX > effDis) {
  		// 鼠标右移
  		 document.getElementById("axis_right").className = "pressed"
   		map.set("mousemoveright","1")

	} else if (lastX > e.clientX && lastX - e.clientX > effDis) {
  		// 鼠标左移
  		document.getElementById("axis_left").className = "pressed"
  		map.set("mousemoveleft","1")
	}
	if (lastY < e.clientY && e.clientY - lastY > effDis) {
  		// 鼠标下移
  		document.getElementById("axis_down").className = "pressed"
  		map.set("mousemovedown","1")
   
	} else if (lastY > e.clientY && lastY - e.clientY > effDis) {
  		// 鼠标上移
  		document.getElementById("axis_up").className = "pressed"
   		map.set("mousemoveup","1")
	}

  lastX = e.clientX;
  lastY = e.clientY;
});

document.addEventListener("mouseup", (e) => {
	if (e.button === 0) {
	  	map.set("mouseleft","1");
	} else if (e.button === 1) {
  	 	map.set("mousemiddle","1");
	} else if (e.button === 2) {
  		map.set("mouseright","1");
	}
});


var haveEvents = 'GamepadEvent' in window;
var haveWebkitEvents = 'WebKitGamepadEvent' in window;
var controllers = {};
var rAF = window.mozRequestAnimationFrame ||
  window.webkitRequestAnimationFrame ||
  window.requestAnimationFrame;

function connecthandler(e) {
  addgamepad(e.gamepad);
}
function addgamepad(gamepad) {
  controllers[gamepad.index] = gamepad; 
  var d = document.createElement("div");
  d.setAttribute("id", "controller" + gamepad.index);
  var t = document.createElement("h1");
  t.appendChild(document.createTextNode("gamepad: " + gamepad.id));
  d.appendChild(t);
  var b = document.createElement("div");
  b.className = "buttons";
  for (var i=0; i<gamepad.buttons.length; i++) {
    var e = document.createElement("span");
    e.className = "button";
    //e.id = "b" + i;
    e.innerHTML = i;
    b.appendChild(e);
  }
  d.appendChild(b);
  var a = document.createElement("div");
  a.className = "axes";
  for (i=0; i<gamepad.axes.length; i++) {
    e = document.createElement("meter");
    e.className = "axis";
    //e.id = "a" + i;
    e.setAttribute("min", "-1");
    e.setAttribute("max", "1");
    e.setAttribute("value", "0");
    e.innerHTML = i;
    a.appendChild(e);
  }
  d.appendChild(a);
  document.getElementById("start").style.display = "none";
  document.getElementById("gamepad").appendChild(d);
  rAF(updateStatus);
}

function disconnecthandler(e) {
  removegamepad(e.gamepad);
}

function removegamepad(gamepad) {
  var d = document.getElementById("controller" + gamepad.index);
  document.body.removeChild(d);
  delete controllers[gamepad.index];
}

function updateStatus() {
  scangamepads();
  for (j in controllers) {
    var controller = controllers[j];
    var d = document.getElementById("controller" + j);
    var buttons = d.getElementsByClassName("button");
    for (var i=0; i<controller.buttons.length; i++) {
      var b = buttons[i];
      var val = controller.buttons[i];
      var pressed = val == 1.0;
      var touched = false;
      if (typeof(val) == "object") {
        pressed = val.pressed;
        if ('touched' in val) {
          touched = val.touched;
        }
        val = val.value;
      }
      var pct = Math.round(val * 100) + "%";
      b.style.backgroundSize = pct + " " + pct;
      
      if (pressed) {
      	map.set("button"+i,"1");
        b.className = " button pressed";
      }
      if (touched) {
        b.className = "button touched";
      }
      if(pressed && touched) {
      	b.className = "button pressed touched"
      }
    }

    var axes = d.getElementsByClassName("axis");
    for (var i=0; i<controller.axes.length; i++) {
      var a = axes[i];
	   
      a.innerHTML = i + ": " + controller.axes[i].toFixed(4);
      a.setAttribute("value", controller.axes[i]);
      if( controller.axes[i] != "0" ) {
		map.set("axis_"+i+controller.axes[i],"1")
      }
    
    }
  }
  
 
  var fin = false
  
  var count = 0
  for (let value of map.values()) {
  	count+=1
  	if(count > 78) {
  		//console.log(map)
  		fin = true
  		alert("PASS")
  	    location.reload()
  	}
  }
  
  if(fin === false) {
  	rAF(updateStatus);
  }
  
  //rAF(updateStatus); 
  
}

function scangamepads() {
  var gamepads = navigator.getGamepads ? navigator.getGamepads() : (navigator.webkitGetGamepads ? navigator.webkitGetGamepads() : []);
  for (var i = 0; i < gamepads.length; i++) {
    if (gamepads[i] && (gamepads[i].index in controllers)) {
      controllers[gamepads[i].index] = gamepads[i];
    }
  }
}

if (haveEvents) {
  window.addEventListener("gamepadconnected", connecthandler);
  window.addEventListener("gamepaddisconnected", disconnecthandler);
} else if (haveWebkitEvents) {
  window.addEventListener("webkitgamepadconnected", connecthandler);
  window.addEventListener("webkitgamepaddisconnected", disconnecthandler);
} else {
  setInterval(scangamepads, 500);
}


