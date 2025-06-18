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

        // Update sensor cube rotations
        if (cubeA) cubeA.rotation.set(rollA, pitchA, yawA);
        if (cubeB) cubeB.rotation.set(rollB, pitchB, yawB);
        
        // Control leg movements based on sensor data
        if (window.frontLegGroup && window.hindLegGroup) {
            // Map sensor A (red) to front leg group movement
            // Use pitch for forward/backward leg swing, roll for side tilt
            const frontLegSwing = pitchA * 0.5; // Scale down the movement
            const frontLegTilt = rollA * 0.3;
            
            window.frontLegGroup.rotation.x = frontLegSwing; // Forward/back swing
            window.frontLegGroup.rotation.z = frontLegTilt;  // Side tilt
            
            // Map sensor B (blue) to hind leg group movement  
            const hindLegSwing = pitchB * 0.5;
            const hindLegTilt = rollB * 0.3;
            
            window.hindLegGroup.rotation.x = hindLegSwing; // Forward/back swing
            window.hindLegGroup.rotation.z = hindLegTilt;  // Side tilt
            
            // Add some walking animation based on the difference between sensors
            const walkCycle = (pitchA - pitchB) * 0.2;
            window.frontLegGroup.rotation.y = Math.sin(Date.now() * 0.001 + walkCycle) * 0.1;
            window.hindLegGroup.rotation.y = Math.sin(Date.now() * 0.001 + walkCycle + Math.PI) * 0.1;
        }
     
        

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
        if (renderer && scene && camera) {
            renderer.render(scene, camera);
        }
    }
    

    function initDogModel(containerId, modelUrl) {
        scene = new THREE.Scene();
        camera = new THREE.PerspectiveCamera(75, 1, 0.1, 1000);
        renderer = new THREE.WebGLRenderer({ antialias: true });
        renderer.setSize(400, 400); // Larger since only one dog
        renderer.setClearColor(0x222222); // Set a dark gray background instead of black
        document.getElementById(containerId).appendChild(renderer.domElement);
    
        // Add ambient light for overall illumination
        const ambientLight = new THREE.AmbientLight(0x404040, 0.8);
        scene.add(ambientLight);
        
        // Add directional light from multiple angles
        const directionalLight1 = new THREE.DirectionalLight(0xffffff, 0.6);
        directionalLight1.position.set(5, 5, 5);
        scene.add(directionalLight1);
        
        const directionalLight2 = new THREE.DirectionalLight(0xffffff, 0.4);
        directionalLight2.position.set(-5, 3, -5);
        scene.add(directionalLight2);
        
        // Add point light for better illumination
        const pointLight = new THREE.PointLight(0xffffff, 0.5, 100);
        pointLight.position.set(0, 3, 0);
        scene.add(pointLight);
        
        // Position camera closer for better view of the stick figure dog
        camera.position.set(1.2, 0.8, 1.8); // Much closer - about 3x zoom in
        camera.lookAt(0, 0.3, 0); // Look at the dog's body center
    
        // Create stick figure dog model with controllable legs
        console.log('Initializing stick figure dog model...');
        dog = new THREE.Group();
        
        // Materials
        const bodyMat = new THREE.MeshPhongMaterial({ color: 0x654321 });
        const jointMat = new THREE.MeshPhongMaterial({ color: 0x8B4513 });
        const legMat = new THREE.MeshPhongMaterial({ color: 0x654321 });
        
        // Main body (spine)
        const bodyGeo = new THREE.CylinderGeometry(0.08, 0.08, 1.2);
        const body = new THREE.Mesh(bodyGeo, bodyMat);
        body.rotation.z = Math.PI / 2; // Make horizontal
        body.position.set(0, 0.3, 0);
        dog.add(body);
        
        // Head
        const headGeo = new THREE.SphereGeometry(0.15);
        const head = new THREE.Mesh(headGeo, jointMat);
        head.position.set(0.7, 0.3, 0);
        dog.add(head);
        
        // Create leg groups that can rotate
        // Front legs (controlled by sensor A - red)
        const frontLegGroup = new THREE.Group();
        frontLegGroup.position.set(0.4, 0.3, 0); // Front of dog
        
        // Front leg joints (shoulders)
        const frontJointL = new THREE.Mesh(new THREE.SphereGeometry(0.06), jointMat);
        frontJointL.position.set(0, 0, 0.2);
        frontLegGroup.add(frontJointL);
        
        const frontJointR = new THREE.Mesh(new THREE.SphereGeometry(0.06), jointMat);
        frontJointR.position.set(0, 0, -0.2);
        frontLegGroup.add(frontJointR);
        
        // Front legs
        const frontLegLGeo = new THREE.CylinderGeometry(0.04, 0.04, 0.6);
        const frontLegL = new THREE.Mesh(frontLegLGeo, legMat);
        frontLegL.position.set(0, -0.3, 0.2);
        frontLegGroup.add(frontLegL);
        
        const frontLegR = new THREE.Mesh(frontLegLGeo, legMat);
        frontLegR.position.set(0, -0.3, -0.2);
        frontLegGroup.add(frontLegR);
        
        dog.add(frontLegGroup);
        
        // Hind legs (controlled by sensor B - blue)
        const hindLegGroup = new THREE.Group();
        hindLegGroup.position.set(-0.4, 0.3, 0); // Back of dog
        
        // Hind leg joints (hips)
        const hindJointL = new THREE.Mesh(new THREE.SphereGeometry(0.06), jointMat);
        hindJointL.position.set(0, 0, 0.2);
        hindLegGroup.add(hindJointL);
        
        const hindJointR = new THREE.Mesh(new THREE.SphereGeometry(0.06), jointMat);
        hindJointR.position.set(0, 0, -0.2);
        hindLegGroup.add(hindJointR);
        
        // Hind legs
        const hindLegLGeo = new THREE.CylinderGeometry(0.04, 0.04, 0.6);
        const hindLegL = new THREE.Mesh(hindLegLGeo, legMat);
        hindLegL.position.set(0, -0.3, 0.2);
        hindLegGroup.add(hindLegL);
        
        const hindLegR = new THREE.Mesh(hindLegLGeo, legMat);
        hindLegR.position.set(0, -0.3, -0.2);
        hindLegGroup.add(hindLegR);
        
        dog.add(hindLegGroup);
        
        // Tail
        const tailGeo = new THREE.CylinderGeometry(0.03, 0.02, 0.3);
        const tail = new THREE.Mesh(tailGeo, bodyMat);
        tail.position.set(-0.7, 0.4, 0);
        tail.rotation.z = -Math.PI / 6; // Slightly angled up
        dog.add(tail);
        
        scene.add(dog);
        
        // Store leg group references for animation
        window.frontLegGroup = frontLegGroup;
        window.hindLegGroup = hindLegGroup;
        
        // Create sensor cubes positioned above the leg groups
        const geoA = new THREE.BoxGeometry(0.12, 0.12, 0.12);
        const matA = new THREE.MeshBasicMaterial({ color: 0xff0000, transparent: true, opacity: 0.8 });
        cubeA = new THREE.Mesh(geoA, matA);
        cubeA.position.set(0, 0.15, 0); // Above front legs
        frontLegGroup.add(cubeA);
        
        const geoB = new THREE.BoxGeometry(0.12, 0.12, 0.12);
        const matB = new THREE.MeshBasicMaterial({ color: 0x0000ff, transparent: true, opacity: 0.8 });
        cubeB = new THREE.Mesh(geoB, matB);
        cubeB.position.set(0, 0.15, 0); // Above hind legs
        hindLegGroup.add(cubeB);
        
        console.log('Created stick figure dog model with controllable legs');
        console.log('Stick figure model: Red sensor controls front legs, Blue sensor controls hind legs');
        console.log('Leg movement based on gyro pitch (forward/back) and roll (tilt)');
        
        // Start animation
        animate();
    
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
