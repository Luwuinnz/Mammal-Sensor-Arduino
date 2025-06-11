document.addEventListener("DOMContentLoaded", function () {
    Chart.register(ChartStreaming);

    const ctx = document.getElementById('liveChart').getContext('2d');

    const chart = new Chart(ctx, {
        type: 'line',
        data: {
            datasets: [
                { label: 'AccX', borderColor: 'red', data: [], pointRadius: 0, pointHoverRadius: 0 },
                { label: 'AccY', borderColor: 'green', data: [], pointRadius: 0, pointHoverRadius: 0 },
                { label: 'AccZ', borderColor: 'blue', data: [], pointRadius: 0, pointHoverRadius: 0 }
            ]
        },
        options: {
            responsive: true,
            animation: false,
            interaction: {
                mode: 'nearest',
                intersect: false
            },
            scales: {
                x: {
                    type: 'realtime',
                    realtime: {
                        duration: 20000,  // show last 20 seconds
                        refresh: 100,     // refresh every 100ms
                        delay: 500,       // buffer delay
                        pause: false
                    },
                    title: {
                        display: true,
                        text: 'Time (s)',
                        color: 'black'
                    }
                },
                y: {
                    min: -3,
                    max: 3,
                    title: {
                        display: true,
                        text: 'MPU A: Acceleration (g)',
                        color: 'black'
                    },
                    ticks: {
                        color: 'black'
                    }
                }
            }
        }
    });


    const ctx2 = document.getElementById('liveChart2').getContext('2d');

    const chart2 = new Chart(ctx2, {
        type: 'line',
        data: {
            datasets: [
                { label: 'AccX', borderColor: 'red', data: [], pointRadius: 0, pointHoverRadius: 0 },
                { label: 'AccY', borderColor: 'green', data: [], pointRadius: 0, pointHoverRadius: 0 },
                { label: 'AccZ', borderColor: 'blue', data: [], pointRadius: 0, pointHoverRadius: 0 }
            ]
        },
        options: {
            responsive: true,
            animation: false,
            interaction: {
                mode: 'nearest',
                intersect: false
            },
            scales: {
                x: {
                    type: 'realtime',
                    realtime: {
                        duration: 20000,
                        refresh: 100,
                        delay: 500,
                        pause: false
                    },
                    title: {
                        display: true,
                        text: 'Time (s)',
                        color: 'black'
                    }
                },
                y: {
                    min: -3,
                    max: 3,
                    title: {
                        display: true,
                        text: 'MPU B: Acceleration (g)',
                        color: 'black'
                    },
                    ticks: {
                        color: 'black'
                    }
                }
            }
        }
    });


    const socket = new WebSocket("ws://" + location.hostname + ":81/");

    socket.onmessage = function (event) {
        let parts = event.data.split(",");
        if (parts.length < 24) return;

        let accX = parseFloat(parts[2]);
        let accY = parseFloat(parts[3]);
        let accZ = parseFloat(parts[4]);

        let accX2 = parseFloat(parts[14]);
        let accY2 = parseFloat(parts[15]);
        let accZ2 = parseFloat(parts[16]);

        document.getElementById("rawDataText").textContent = event.data;

        if ([accX, accY, accZ, accX2, accY2, accZ2].some(isNaN)) return;

        const now = Date.now();

        //gryo

        // Update 3D Cube rotations
        let rollA = parseFloat(parts[9]) * Math.PI / 180;
        let pitchA = parseFloat(parts[10]) * Math.PI / 180;
        let yawA = parseFloat(parts[11]) * Math.PI / 180;

        let rollB = parseFloat(parts[21]) * Math.PI / 180;
        let pitchB = parseFloat(parts[22]) * Math.PI / 180;
        let yawB = parseFloat(parts[23]) * Math.PI / 180;

        if (cubeA) cubeA.rotation.set(rollA, pitchA, yawA);
        if (cubeB) cubeB.rotation.set(rollB, pitchB, yawB);
     
        

        // First chart (MPUA)
        chart.data.datasets[0].data.push({ x: now, y: accX });
        chart.data.datasets[1].data.push({ x: now, y: accY });
        chart.data.datasets[2].data.push({ x: now, y: accZ });

        // Second chart (MPUB)
        chart2.data.datasets[0].data.push({ x: now, y: accX2 });
        chart2.data.datasets[1].data.push({ x: now, y: accY2 });
        chart2.data.datasets[2].data.push({ x: now, y: accZ2 });

        console.log("MPU A rotation (radians):", rollA, pitchA, yawA);
        console.log("MPU B rotation (radians):", rollB, pitchB, yawB);

    };

    // Toggle Logging
    const toggleBtn = document.getElementById("toggleLoggingBtn");
    const loggingStatus = document.getElementById("loggingStatus");
    let loggingEnabled = true;

    function updateLoggingUI() {
        toggleBtn.textContent = loggingEnabled ? "Pause Logging" : "Resume Logging";
        loggingStatus.textContent = loggingEnabled ? "Logging..." : "Logging paused.";
        toggleBtn.classList.toggle("logging-on", loggingEnabled);
        toggleBtn.classList.toggle("logging-off", !loggingEnabled);
    }

    // --- 3D Cube Setup ---
    let dog, cubeA, cubeB;
    let scene, camera, renderer;

    function animate() {
        requestAnimationFrame(animate);
        if (dog) renderer.render(scene, camera);
    }
    //animate();
    

    function initDogModel(containerId, modelUrl) {
        scene = new THREE.Scene();
        camera = new THREE.PerspectiveCamera(75, 1, 0.1, 1000);
        renderer = new THREE.WebGLRenderer({ antialias: true });
        renderer.setSize(400, 400); // Larger since only one dog
        document.getElementById(containerId).appendChild(renderer.domElement);
    
        const light = new THREE.DirectionalLight(0xffffff, 1);
        light.position.set(2, 2, 5);
        scene.add(light);
    
        const loader = new THREE.GLTFLoader();
        let spine005Found = false, spine009Found = false;
        loader.load(modelUrl, function (gltf) {
            dog = gltf.scene;
            dog.scale.set(1.5, 1.5, 1.5);
            scene.add(dog);
    
            gltf.scene.traverse((child) => {
                if (child.isBone) {

                        console.log("Found bone:", child.name);  // <-- ADD THIS LINE
                
                    if (child.name === "spine005_34") {
                        spine005Found = true;
                        const geo = new THREE.BoxGeometry(0.1, 0.1, 0.1);
                        const mat = new THREE.MeshBasicMaterial({ color: 0xff0000, transparent: true, opacity: 0.5 });
                        cubeA = new THREE.Mesh(geo, mat);
                        cubeA.position.set(0, 0.2, 0);
                        child.add(cubeA);
                    }
                    if (child.name === "spine009_8") {
                        spine009Found = true;
                        const geo = new THREE.BoxGeometry(0.1, 0.1, 0.1);
                        const mat = new THREE.MeshBasicMaterial({ color: 0x0000ff, transparent: true, opacity: 0.5 });
                        cubeB = new THREE.Mesh(geo, mat);
                        cubeB.position.set(0, 0.2, 0);
                        child.add(cubeB);
                    }
                }
            });


            if (!spine005Found || !spine009Found) {
                console.warn("Bone(s) not found:", { spine005Found, spine009Found });
            }
    
            animate();
        });
    
        camera.position.z = 5;
    
    }
    
    
    

    window.addEventListener("load", () => {
        initDogModel("rendererA", "/dog.glb");
        updateLoggingUI();
    
        fetch("/loggingStatus")
            .then(res => res.text())
            .then(state => {
                loggingEnabled = (state.trim() === "on");
                updateLoggingUI();
            });
    
        toggleBtn.addEventListener("click", () => {
            loggingEnabled = !loggingEnabled;
            fetch(`/toggleLogging?state=${loggingEnabled ? "on" : "off"}`)
                .then(res => res.ok ? updateLoggingUI() : alert("Failed to toggle logging on ESP32"))
                .catch(err => console.error("Toggle error:", err));
        });
    });
    
    


    toggleBtn.addEventListener("click", () => {
        loggingEnabled = !loggingEnabled;
        fetch(`/toggleLogging?state=${loggingEnabled ? "on" : "off"}`)
            .then(res => res.ok ? updateLoggingUI() : alert("Failed to toggle logging on ESP32"))
            .catch(err => console.error("Toggle error:", err));
    });

    fetch("/loggingStatus")
        .then(res => res.text())
        .then(state => {
            loggingEnabled = (state.trim() === "on");
            updateLoggingUI();
        });

    updateLoggingUI(); // Init
});
