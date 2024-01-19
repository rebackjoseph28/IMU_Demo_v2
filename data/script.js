import * as THREE from './three.js';
import { OBJLoader } from './OBJLoader.js';

const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
const renderer = new THREE.WebGLRenderer();
let F22;
let rotx = 0, roty = 0, rotz = 0;
const degToRad = (Math.PI/180);

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
// Init web socket when the page loads
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getReadings(){
    websocket.send("getReadings");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

// When websocket is established, call the getReadings() function
function onOpen(event) {
    console.log('Connection opened');
    getReadings();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

// Function that receives the message from the ESP32 with the readings
function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    for (var i = 0; i < keys.length; i++) {
        var key = keys[i];
        var element = document.getElementById(key);
        
        //somehow x and y are swapped
        if (key.localeCompare("absX") === 0) {
            rotx = myObj[key]*4*degToRad;
            console.warn("key absX found");
        }
        if (key.localeCompare("absY") === 0) {
            roty = -myObj[key]*4*degToRad;
            console.warn("key absY found");
        }
        if (key.localeCompare("absZ") === 0) {
             rotz = myObj[key]*4*degToRad;
             console.warn("key absZ found");
        }
        
        if (element) {
            element.innerHTML = myObj[key];
        } else {
            console.warn(`Element with ID '${key}' not found.`);
        }
    }
}


// Initialize the scene
function init() {
    // Create a camera
    camera.position.z = 0.25;

    // Create a renderer
    renderer.setSize(window.innerWidth, window.innerHeight);
    document.getElementById("scene-container").appendChild(renderer.domElement);

    // Load the OBJ file
    const loader = new OBJLoader();
    loader.load('/F22-Demo.obj', function (object) {
        // Store the loaded OBJ in a variable for later manipulation
        F22 = object;

        // Add the loaded OBJ to the scene
        scene.add(F22);

        // Start the animation once the object is loaded
        animate();
    });

    // Add lights
    const ambientLight = new THREE.AmbientLight(0x404040);
    scene.add(ambientLight);

    const directionalLight = new THREE.DirectionalLight(0xffffff, 0.5);
    directionalLight.position.set(0, 1, 0);
    scene.add(directionalLight);
    }

        // Render the scene
    function animate() {
        requestAnimationFrame(animate);

        // Rotate the object
        if (F22) {
            F22.rotation.z = rotz;
            F22.rotation.y = roty;
            F22.rotation.x = rotx;
        }

        // Render the scene
        renderer.render(scene, camera);
    }

        // Resize the viewport when the window size changes
    window.addEventListener('resize', function () {
        const newWidth = window.innerWidth;
        const newHeight = window.innerHeight;

        camera.aspect = newWidth / newHeight;
        camera.updateProjectionMatrix();

        renderer.setSize(newWidth, newHeight);
    });

// Initialize the scene and start rendering
init();
animate();