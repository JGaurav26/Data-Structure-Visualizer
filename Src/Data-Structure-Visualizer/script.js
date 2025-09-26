let array = [];

// Initialize Chart.js
const ctx = document.getElementById('bubbleSortChart').getContext('2d');
const bubbleSortChart = new Chart(ctx, {
    type: 'bar',
    data: {
        labels: [],
        datasets: [{
            label: 'Array Elements',
            data: [],
            backgroundColor: 'rgba(52, 152, 219, 0.6)',
        }],
    },
    options: {
        scales: {
            y: {
                beginAtZero: true,
                title: {
                    display: true,
                    text: 'Values'
                }
            },
            x: {
                title: {
                    display: true,
                    text: 'Index'
                }
            }
        }
    }
});

// Function to create a new array from user input
function createArray() {
    const input = document.getElementById('array-input').value;
    array = input.split(',').map(Number);
    drawArray();
}

// Function to generate a random array
function generateRandomArray() {
    array = Array.from({ length: 10 }, () => Math.floor(Math.random() * 100));
    drawArray();
}

// Function to draw the array on the screen
function drawArray() {
    const arrayContainer = document.getElementById('array-container');
    arrayContainer.innerHTML = '';
    array.forEach((number, index) => {
        const numberDiv = document.createElement('div');
        numberDiv.className = 'array-number';
        numberDiv.style.left = `${index * 55}px`; // Position based on index
        numberDiv.textContent = number;
        arrayContainer.appendChild(numberDiv);
    });

    // Update chart data
    bubbleSortChart.data.labels = array.map((_, index) => index);
    bubbleSortChart.data.datasets[0].data = array;
    bubbleSortChart.update();
}

// Function to visualize Bubble Sort
async function visualizeBubbleSort() {
    const speedInput = document.getElementById('speed');
    const speed = speedInput.value;

    for (let i = 0; i < array.length - 1; i++) {
        for (let j = 0; j < array.length - i - 1; j++) {
            const arrayNumbers = document.querySelectorAll('.array-number');

            // Highlight both elements being compared
            arrayNumbers[j].classList.add('comparing');
            arrayNumbers[j + 1].classList.add('comparing');

            // Smooth transition delay for visual effect
            await new Promise(resolve => setTimeout(resolve, speed));

            // Swap if elements are in the wrong order
            if (array[j] > array[j + 1]) {
                // Animate the swap
                await animateSwap(arrayNumbers[j], arrayNumbers[j + 1]);

                // Update the array values
                [array[j], array[j + 1]] = [array[j + 1], array[j]];
                drawArray(); // Redraw the array
            }

            // Remove highlighting
            arrayNumbers[j].classList.remove('comparing');
            arrayNumbers[j + 1].classList.remove('comparing');
        }
    }

    // Mark all elements as sorted in green
    markAsSorted();

    // Final update to the chart
    drawArray();
}

// ---------------- Insertion Sort ----------------
async function visualizeInsertionSort() {
    const speed = document.getElementById("speed").value;
    const arrayNumbers = document.querySelectorAll(".array-number");

    for (let i = 1; i < array.length; i++) {
        let key = array[i];
        let j = i - 1;

        while (j >= 0 && array[j] > key) {
            arrayNumbers[j].classList.add("comparing");
            arrayNumbers[j + 1].classList.add("comparing");

            await new Promise(resolve => setTimeout(resolve, speed));

            array[j + 1] = array[j];
            drawArray();

            arrayNumbers[j].classList.remove("comparing");
            arrayNumbers[j + 1].classList.remove("comparing");

            j--;
        }
        array[j + 1] = key;
        drawArray();
    }
    markAsSorted();
}

// ---------------- Merge Sort ----------------
async function visualizeMergeSort() {
    const speed = document.getElementById("speed").value;

    async function mergeSort(arr, l, r) {
        if (l >= r) return;
        let mid = Math.floor((l + r) / 2);

        await mergeSort(arr, l, mid);
        await mergeSort(arr, mid + 1, r);
        await merge(arr, l, mid, r);
    }

    async function merge(arr, l, m, r) {
        let left = arr.slice(l, m + 1);
        let right = arr.slice(m + 1, r + 1);

        let i = 0, j = 0, k = l;

        while (i < left.length && j < right.length) {
            const arrayNumbers = document.querySelectorAll(".array-number");
            arrayNumbers[k].classList.add("comparing");

            await new Promise(resolve => setTimeout(resolve, speed));

            if (left[i] <= right[j]) {
                arr[k++] = left[i++];
            } else {
                arr[k++] = right[j++];
            }
            drawArray();
            arrayNumbers[k - 1].classList.remove("comparing");
        }

        while (i < left.length) arr[k++] = left[i++];
        while (j < right.length) arr[k++] = right[j++];

        drawArray();
    }

    await mergeSort(array, 0, array.length - 1);
    markAsSorted();
}

// ---------------- Quick Sort ----------------
async function visualizeQuickSort() {
    const speed = document.getElementById("speed").value;

    async function quickSort(arr, low, high) {
        if (low < high) {
            let pi = await partition(arr, low, high);
            await quickSort(arr, low, pi - 1);
            await quickSort(arr, pi + 1, high);
        }
    }

    async function partition(arr, low, high) {
        let pivot = arr[high];
        let i = low - 1;

        for (let j = low; j < high; j++) {
            const arrayNumbers = document.querySelectorAll(".array-number");
            arrayNumbers[j].classList.add("comparing");

            await new Promise(resolve => setTimeout(resolve, speed));

            if (arr[j] < pivot) {
                i++;
                [arr[i], arr[j]] = [arr[j], arr[i]];
                drawArray();
            }

            arrayNumbers[j].classList.remove("comparing");
        }

        [arr[i + 1], arr[high]] = [arr[high], arr[i + 1]];
        drawArray();

        return i + 1;
    }

    await quickSort(array, 0, array.length - 1);
    markAsSorted();
}

// Function to animate the swap of two elements
async function animateSwap(element1, element2) {
    const temp1 = element1.style.left;
    const temp2 = element2.style.left;

    element1.style.left = temp2;
    element2.style.left = temp1;

    await new Promise(resolve => setTimeout(resolve, 400)); // Wait for animation to finish
}

// Function to mark all elements as sorted
function markAsSorted() {
    const arrayNumbers = document.querySelectorAll('.array-number');
    arrayNumbers.forEach((numberDiv) => {
        numberDiv.classList.add('sorted'); // Add 'sorted' class to all elements
    });
}