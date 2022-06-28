<script>

export default {
    data() {
        return {
            inputFilename: "",
            numIterations: 2,
            minSupport: 0.5,
            minConfidence: 0.05,
            numWorkers: 4
        }
    }
}
</script>

<template>
    <div class="NewJob">
        <h1>New Job</h1>
        <form id="new-job-form">
            <div class="form-field">
                <label for="input-filename">Input file:</label>
                <input type="file" id="input-filename" />
                <button>Preview</button>
            </div>

            <div class="form-field">
                <label for="max-k">Iterations:</label>
                <input type="number" id="max-k" v-model="numIterations" />
            </div>

            <div class="form-field">
                <label for="min-sup">Min. Support:</label>
                <input type="number" id="min-sup" v-model="minSupport" />
            </div>

            <div class="form-field">
                <label for="min-conf">Min. Confidence:</label>
                <input type="number" id="min-conf" v-model="minConfidence" />
            </div>

            <div class="form-field">
                <label for="worker-size">Number of workers:</label>
                <select v-model="numWorkers" id="worker-size">
                    <option value="2">2</option>
                    <option value="4">4</option>
                    <option value="8">8</option>
                    <option value="16">16</option>
                </select>
            </div>

            <input type="submit" value="Submit" id="submit-btn" />
        </form>
    </div>
</template>

<style>
#new-job-form {
    padding: 1rem;
    margin: 1rem;
}

.form-field {
    margin: 0.5em 0;
}

#submit-btn {
    margin: 0.5rem 0;
    width: auto;
    cursor: pointer;
    border-radius: 5px;
}
</style>
