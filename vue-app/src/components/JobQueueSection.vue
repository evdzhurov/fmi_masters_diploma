<script>
import JobQueueItem from './JobQueueItem.vue'
export default {
    components: {
        JobQueueItem
    },
    data() {
        return {
            jobs: {
                "1" : {id: "1", status: "Pending"},
                "2" : {id: "2", status: "Pending"},
                "3" : {id: "3", status: "Pending"},
                "4" : {id: "4", status: "Pending"},
                "5" : {id: "5", status: "Pending"},
            },
        }
    },
    methods: {
        removeJob(id) {
            delete this.jobs[id]
        }
    }
}
</script>

<template>
    <div class="JobQueue">
        <h1>Job Queue</h1>
        <p>Free workers: 8/8</p>
        <JobQueueItem v-for="job in jobs" 
            :key="job.id" 
            :job=job
            @remove-job="removeJob"
        ></JobQueueItem>
    </div>
</template>
