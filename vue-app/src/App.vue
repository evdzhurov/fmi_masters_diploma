<template>
  <div class="Dashboard">
    <HeaderSection />
    <NewJobSection />
    <JobQueueSection />
    <AboutSection />
  </div>
</template>

<script>
import HeaderSection from './components/HeaderSection.vue'
import NewJobSection from './components/NewJobSection.vue'
import JobQueueSection from './components/JobQueueSection.vue'
import AboutSection from './components/AboutSection.vue'

export default {
  name: 'App',
  components: {
    HeaderSection,
    NewJobSection,
    JobQueueSection,
    AboutSection
  }
}
</script>

<style>
#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  color: #2c3e50;
}

.Dashboard {
  box-sizing: border-box;
  padding: 25px;
  display: grid;
  height: 100vh;
  grid-gap: 25px;
  grid-template-rows: 1fr 4fr 1fr;
  grid-template-columns: 3fr 2fr;
  grid-template-areas:
    "Header Header"
    "NewJob JobQueue"
    "About About"
}

.Dashboard div {
  border-radius: 10px;
}

.Dashboard h1 {
  text-align: center;
}

.Header {
  grid-area: Header;
  background-color: lightblue;
}

.NewJob {
  grid-area: NewJob;
  background-color: palegreen;
}

.JobQueue {
  grid-area: JobQueue;
  background-color: palegoldenrod;
}

.About {
  grid-area: About;
  background-color: lightgray;
}
</style>
