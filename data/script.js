import * as THREE from './three.js';
import { OBJLoader } from './OBJLoader.js';

const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
const renderer = new THREE.WebGLRenderer();
let F22;
let rotx = 0, roty = 0, rotz = 0;
let gyrox = 0, gyroy = 0, gyroz = 0;
//let rotation = new THREE.Quaternion();
let magx = 0, magy = 0, magz = 0;
let accx = 0, accy = 0, accz = 0;
const degToRad = (2 * Math.PI / (360));
const radoffset = 1/2;
const reset = new THREE.Euler();

// FOR THE MAGNETIC VECTOR
const magPoints = [];
magPoints.push(new THREE.Vector3(0, 0, 0)); // Start point
magPoints.push(new THREE.Vector3(magx, magy, magz)); // End point
const lineGeometry = new THREE.BufferGeometry().setFromPoints(magPoints);
const lineMaterial = new THREE.LineBasicMaterial({ color: 0xff0000 });

//FOR THE ACCELERATION VECTOR
const accPoints = [];
accPoints.push(new THREE.Vector3(0, 0, 0)); // Start point
accPoints.push(new THREE.Vector3(accx, accy, accz)); // End point
const lineGeometry2 = new THREE.BufferGeometry().setFromPoints(magPoints);
const lineMaterial2 = new THREE.LineBasicMaterial({ color: 0x00ff00 });

let accNorm = new THREE.Vector3(0,0,0);
let rotQuat = new THREE.Quaternion(0,0,0,0);

const material = new THREE.LineBasicMaterial({
    color: 0xffffff,
    linewidth: 1,
    linecap: 'round', //ignored by WebGLRenderer
    linejoin: 'round' //ignored by WebGLRenderer
});

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
// Init web socket when the page loads
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getReadings() {
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

        //somehow y is inverted
        if (key.localeCompare("absX") === 0) rotx = parseFloat(myObj[key]) * degToRad;
        if (key.localeCompare("absY") === 0) roty = -parseFloat(myObj[key]) * degToRad;
        if (key.localeCompare("absZ") === 0) rotz = parseFloat(myObj[key]) * degToRad;

        if (key.localeCompare("gyrox") === 0){
            gyrox = parseFloat(myObj[key]) * degToRad * radoffset;
            if (gyrox < 0.005 && gyrox > -0.005) gyrox = 0;
        } 
        if (key.localeCompare("gyroy") === 0){
            gyroy = -parseFloat(myObj[key]) * degToRad * radoffset;
            if (gyroy < 0.005 && gyroy > -0.005) gyroy = 0;
        }
        if (key.localeCompare("gyroz") === 0) {
            gyroz = parseFloat(myObj[key]) * degToRad * radoffset;
            if (gyroz < 0.005 && gyroz > -0.005) gyroz = 0;
        }
        if (key.localeCompare("magx") === 0) magx = parseFloat(myObj[key]);
        if (key.localeCompare("magy") === 0) magy = parseFloat(myObj[key]);
        if (key.localeCompare("magz") === 0) magz = parseFloat(myObj[key]);

        if (key.localeCompare("accelx") === 0) accx = parseFloat(myObj[key]);
        if (key.localeCompare("accely") === 0) accy = parseFloat(myObj[key]);
        if (key.localeCompare("accelz") === 0) accz = parseFloat(myObj[key]);

        if (element) {
            element.innerHTML = myObj[key];
        } else {
            console.warn(`Element with ID '${key}' not found.`);
        }
    }
}

function generateQuaternion(rx,ry,rz){
    let w = Math.sqrt(rx^2+ry^2+rz^2);
    const qgyro = new THREE.Quaternion((rx/2), (ry/2), (rz/2),1);
    qgyro.normalize();
    return qgyro;
}

document.getElementById("cal").addEventListener("click", calibrate);
function calibrate(){
    F22.setRotationFromEuler(reset);
}

// Initialize the scene
function init() {
    // Create a camera
    camera.position.z = 0.25;

    // Create a renderer
    renderer.setSize(window.innerWidth, window.innerHeight);
    document.getElementById("scene-container").appendChild(renderer.domElement);

    const loaderT = new THREE.TextureLoader();
    const texture = loaderT.load(
        '/sky.jpg',
        () => {
            texture.mapping = THREE.EquirectangularReflectionMapping;
            texture.colorSpace = THREE.SRGBColorSpace;
            scene.background = texture;
        });
    
    // create quaternion from acceleration vector
    accNorm = new THREE.Vector3(accx,accy,accz);
    accNorm.normalize();

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

    // Create the line and add it to the scene
    const magLine = new THREE.Line(lineGeometry, lineMaterial);
    scene.add(magLine);

    const accLine = new THREE.Line(lineGeometry2, lineMaterial2);
    scene.add(accLine);

    const rotQuat = generateQuaternion(gyrox,gyroy,gyroz);

    // Add lights
    const ambientLight = new THREE.AmbientLight(0x404040);
    scene.add(ambientLight);

    const directionalLight = new THREE.DirectionalLight(0xffffff, 0.5);
    directionalLight.position.set(0, 1, 1);
    scene.add(directionalLight);

    window.addEventListener('resize', onWindowResize);
}

function onWindowResize() {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
}

// Render the scene
function animate() {
    requestAnimationFrame(animate);

    magPoints[1].set(magx,magy,magz);
    lineGeometry.setFromPoints(magPoints);

    accPoints[1].set(accx,accy,accz);
    lineGeometry2.setFromPoints(accPoints);

    // Rotate the object
    if (F22) {
        rotQuat.set((gyrox/2), (gyroy/2), (gyroz/2), 1);
        rotQuat.normalize();
        F22.applyQuaternion(rotQuat);
        //F22.rotation.z = rotz;
        //F22.rotation.y = roty;
        //F22.rotation.x = rotx;
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