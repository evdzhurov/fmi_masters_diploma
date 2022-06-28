<script>
export default {
    props: {
        job: Object
    },
}
</script>

<template>
    <li>Job {{job.id}} | {{job.status}} <button>Input</button> <button>Result</button> <button @click="$emit('removeJob', job.id)">Remove</button></li>
</template>