def notify(status){
	emailext (
		body: '$DEFAULT_CONTENT',
		recipientProviders: [
			[$class: 'CulpritsRecipientProvider'],
			[$class: 'DevelopersRecipientProvider'],
			[$class: 'RequesterRecipientProvider']
		],
		replyTo: '$DEFAULT_REPLYTO',
		subject: '$DEFAULT_SUBJECT',
		to: '$DEFAULT_RECIPIENTS'
	)
}

@NonCPS
def killall_jobs() {
	def jobname = env.JOB_NAME
	def buildnum = env.BUILD_NUMBER.toInteger()
	def killnums = ""
	def job = Jenkins.instance.getItemByFullName(jobname)
	def fixed_job_name = env.JOB_NAME.replace('%2F','/')

	for (build in job.builds) {
		if (!build.isBuilding()) { continue; }
		if (buildnum == build.getNumber().toInteger()) { continue; println "equals" }
		if (buildnum < build.getNumber().toInteger()) { continue; println "newer" }

		echo "Kill task = ${build}"

		killnums += "#" + build.getNumber().toInteger() + ", "

		build.doStop();
	}

	if (killnums != "") {
		discordSend description: "in favor of #${buildnum}, ignore following failed builds for ${killnums}", footer: "", link: env.BUILD_URL, result: "ABORTED", title: "[${split_job_name[0]}] Killing task(s) ${fixed_job_name} ${killnums}", webhookURL: env.AMIGADEV_WEBHOOK
	}
	echo "Done killing"
}

def buildStep() {
	def split_job_name = env.JOB_NAME.split(/\/{1}/);
	def fixed_job_name = split_job_name[1].replace('%2F',' ');

	try{
		stage("Building on \"amigadev/crosstools:m68k-amigaos\" for \"Amiga Classic\"...") {
			properties([pipelineTriggers([githubPush()])])
			def commondir = env.WORKSPACE + '/../' + fixed_job_name + '/'

			def dockerImageRef = docker.image("amigadev/crosstools:m68k-amigaos");
			dockerImageRef.pull();

			checkout scm;

			dockerImageRef.inside("") {

				if (env.CHANGE_ID) {
					echo 'Trying to build pull request'
				}

				sh "mkdir -p lib/"
				sh "rm -rfv build-68k/*"

				stage("Building...") {
					sh "cmake -S . -B build-68k -DCMAKE_INSTALL_PREFIX=./install -DCMAKE_BUILD_TYPE=Release"
					sh "cmake --build build-68k --config Release --target install -j 8"
					
					sh "cd install && lha -c ../ace.lha ./*"

					archiveArtifacts artifacts: '*.zip,*.tar.gz,*.tgz,*.lha'
				}


				discordSend description: "Target: Amiga Classic DockerImage: amigadev/crosstools:m68k-amigaos successful!", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Build Successful: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.AMIGADEV_WEBHOOK
			}
		}
	} catch(err) {
		currentBuild.result = 'FAILURE'

		discordSend description: "Build Failed: ${fixed_job_name} #${env.BUILD_NUMBER} Target: Amiga Classic DockerImage: amigadev/crosstools:m68k-amigaos (<${env.BUILD_URL}|Open>)", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Build Failed: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.AMIGADEV_WEBHOOK
		notify('Build failed')
		throw err
	}
}

node('master') {
	killall_jobs();
	def split_job_name = env.JOB_NAME.split(/\/{1}/);
	def fixed_job_name = split_job_name[1].replace('%2F',' ');
	checkout(scm);
	
	env.COMMIT_MSG = sh (
		script: 'git log -1 --pretty=%B ${GIT_COMMIT}',
		returnStdout: true
	).trim();

	discordSend description: "${env.COMMIT_MSG}", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Build Started: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.AMIGADEV_WEBHOOK

	def branches = [:]

	branches["Build ACE"] = {
		node {
			buildStep()
		}
	}

	sh "rm -rf ./*"

	parallel branches;
}
