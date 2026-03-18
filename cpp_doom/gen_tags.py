import re

def translate(eng):
    if eng == "<모두>": return "모든 리소스"
    mapping = {
        "EC2Instance": "EC2 인스턴스",
        "EC2VPC": "VPC (가상 네트워크)",
        "S3Bucket": "S3 버킷",
        "RDSInstance": "RDS 데이터베이스",
        "IAMUser": "IAM 사용자",
        "LambdaFunction": "Lambda 함수",
        "EC2InternetGateway": "인터넷 게이트웨이",
        "EC2Subnet": "VPC 서브넷",
        "EC2SecurityGroup": "보안 그룹",
        "EC2RouteTable": "라우팅 테이블",
        "EC2Volume": "EBS 볼륨",
        "EC2Snapshot": "EBS 스냅샷",
        "S3Object": "S3 객체 (파일)",
        "DynamoDBTable": "DynamoDB 테이블",
        "SQSQueue": "SQS 대기열",
        "SNSTopic": "SNS 토픽",
        "CloudWatchLogsLogGroup": "CloudWatch 로그 그룹",
        "IAMRole": "IAM 역할",
        "IAMPolicy": "IAM 정책",
        "Route53HostedZone": "Route 53 호스팅 영역",
        "ELB": "Classic 로드 밸런서",
        "ELBv2": "Application/Network 로드 밸런서",
        "Certificate": "인증서",
    }
    if eng in mapping: return mapping[eng]
    kor = eng
    rules = [
        (r"Instance$", " 인스턴스"), (r"Bucket$", " 버킷"), (r"Function$", " 함수"),
        (r"User$", " 사용자"), (r"Role$", " 역할"), (r"Policy$", " 정책"),
        (r"Group$", " 그룹"), (r"Table$", " 테이블"), (r"Queue$", " 큐(대기열)"),
        (r"Topic$", " 토픽"), (r"Snapshot$", " 스냅샷"), (r"Cluster$", " 클러스터"),
        (r"Subnet$", " 서브넷"), (r"Gateway$", " 게이트웨이"), (r"RouteTable$", " 라우팅 테이블"),
        (r"SecurityGroup$", " 보안 그룹"), (r"Address$", " 공인 IP(EIP)"), (r"Volume$", " 볼륨"),
        (r"Image$", " 이미지"), (r"KeyPair$", " 키 페어"), (r"LogGroup$", " 로그 그룹"),
        (r"Alarm$", " 알람"), (r"Pipeline$", " 파이프라인"), (r"Repository$", " 저장소"),
        (r"Certificate$", " 인증서"), (r"Secret$", " 시크릿"), (r"Connection$", " 연결"),
        (r"Domain$", " 도메인"), (r"Distribution$", " 배포(CDN)"), (r"Stack$", " 스택"),
        (r"TargetGroup$", " 타겟 그룹"), (r"LoadBalancer$", " 로드 밸런서"), (r"AccessPoint$", " 액세스 포인트"),
        (r"VpcLink$", " VPC 링크"), (r"VpcEndpoint$", " VPC 엔드포인트"), (r"Service$", " 서비스"),
        (r"Project$", " 프로젝트"), (r"Config$", " 설정"), (r"Application$", " 애플리케이션"),
        (r"Environment$", " 환경"), (r"Database$", " 데이터베이스"), (r"NetworkInterface$", " 네트워크 인터페이스 (ENI)"),
    ]
    for pat, rep in rules:
        if re.search(pat, kor):
            kor = re.sub(pat, rep, kor)
            break
    prefixes = ["EC2", "S3", "IAM", "RDS", "Lambda", "VPC", "CloudWatch", "Route53", "SNS", "SQS", "ACM", "EKS", "ECS", "EBS", "ELB", "KMS"]
    for p in prefixes:
        if kor.startswith(p) and len(kor) > len(p) and kor[len(p)].isupper():
            kor = p + " " + kor[len(p):]
    return kor

def get_tags(eng, kor):
    eng_tags = ["#AWS", "#Cloud", "#Infrastructure"]
    kor_tags = ["#AWS", "#클라우드", "#인프라"]
    service_map = {
        "EC2": (["#Compute", "#Server", "#VM", "#Virtual"], ["#컴퓨트", "#서버", "#가상머신", "#가상화"]),
        "S3": (["#Storage", "#Object", "#File", "#Bucket"], ["#스토리지", "#객체", "#파일", "#버킷"]),
        "IAM": (["#Security", "#Access", "#Identity", "#Auth"], ["#보안", "#액세스", "#인증", "#권한"]),
        "RDS": (["#Database", "#SQL", "#Relational", "#DB"], ["#데이터베이스", "#SQL", "#관계형", "#DB"]),
        "Lambda": (["#Serverless", "#Function", "#Code", "#Compute"], ["#서버리스", "#함수", "#코드", "#컴퓨트"]),
        "VPC": (["#Network", "#VPC", "#Connectivity", "#Private"], ["#네트워크", "#VPC", "#연결", "#프라이빗"]),
        "CloudWatch": (["#Logging", "#Monitoring", "#Metrics", "#Alert"], ["#로그", "#모니터링", "#메트릭", "#알람"]),
        "Route53": (["#DNS", "#Domain", "#Network", "#Route"], ["#DNS", "#도메인", "#네트워크", "#라우팅"]),
        "DynamoDB": (["#NoSQL", "#Database", "#KV", "#Serverless"], ["#NoSQL", "#데이터베이스", "#키벨류", "#서버리스"]),
        "SQS": (["#Queue", "#Messaging", "#Async", "#Decoupled"], ["#대기열", "#메시징", "#비동기", "#디커플링"]),
        "SNS": (["#Notification", "#PubSub", "#Messaging", "#Alert"], ["#알림", "#푸시", "#메시징", "#발행구독"]),
        "KMS": (["#Encryption", "#Security", "#Key", "#Management"], ["#암호화", "#보안", "#키", "#관리"]),
        "Glue": (["#ETL", "#Data", "#Analytics", "#Catalog"], ["#ETL", "#데이터", "#분석", "#카탈로그"]),
        "AppStream": (["#Streaming", "#Application", "#Desktop", "#VDA"], ["#스트리밍", "#애플리케이션", "#데스크탑", "#가상화"]),
        "Cognito": (["#Auth", "#Identity", "#User", "#Web"], ["#인증", "#사용자", "#고객", "#웹"]),
        "WAF": (["#Security", "#Firewall", "#Web", "#Network"], ["#보안", "#방화벽", "#웹", "#네트워크"]),
    }
    for prefix, (e_tags, k_tags) in service_map.items():
        if eng.upper().startswith(prefix.upper()):
            eng_tags.extend(e_tags)
            kor_tags.extend(k_tags)
            break
    type_rules = [
        (r"Instance$", (["#Runtime", "#Computing", "#Instance"], ["#실행환경", "#컴퓨팅", "#인스턴스"])),
        (r"Bucket$", (["#Storage", "#Files", "#Bucket"], ["#저장소", "#파일", "#버킷"])),
        (r"Policy$", (["#Permission", "#Rule", "#Policy"], ["#권한", "#규칙", "#정책"])),
        (r"Group$", (["#Cluster", "#Logical", "#Group"], ["#클러스터", "#논리", "#그룹"])),
        (r"Table$", (["#Data", "#Record", "#Table"], ["#데이터", "#레코드", "#테이블"])),
    ]
    for pat, (e_tags, k_tags) in type_rules:
        if re.search(pat, eng):
            eng_tags.extend(e_tags)
            kor_tags.extend(k_tags)
            break
    while len(eng_tags) < 10: eng_tags.append("#Tag" + str(len(eng_tags)))
    while len(kor_tags) < 10: kor_tags.append("#태그" + str(len(kor_tags)))
    all_tags = sorted(list(set(eng_tags[:10]))) + sorted(list(set(kor_tags[:10])))
    return " ".join(all_tags)

def generate_header():
    raw_list = """ACMCertificate
ACMPCACertificateAuthority
ACMPCACertificateAuthorityState
AMGWorkspace
AMPWorkspace
APIGatewayAPIKey
APIGatewayClientCertificate
APIGatewayDomainName
APIGatewayRestAPI
APIGatewayUsagePlan
APIGatewayV2API
APIGatewayV2VpcLink
APIGatewayVpcLink
AWS::AppFlow::ConnectorProfile
AWS::AppFlow::Flow
AWS::AppRunner::Service
AWS::ApplicationInsights::Application
AWS::Backup::Framework
AWS::ECR::PublicRepository
AWS::ECR::PullThroughCacheRule
AWS::ECR::RegistryPolicy
AWS::ECR::ReplicationConfiguration
AWS::MWAA::Environment
AWS::NetworkFirewall::Firewall
AWS::NetworkFirewall::FirewallPolicy
AWS::NetworkFirewall::RuleGroup
AWS::Synthetics::Canary
AWS::Timestream::Database
AWS::Timestream::ScheduledQuery
AWS::Timestream::Table
AWS::Transfer::Workflow
AWSBackupPlan
AWSBackupRecoveryPoint
AWSBackupSelection
AWSBackupVault
AWSBackupVaultAccessPolicy
AccessAnalyzer
AppMeshGatewayRoute
AppMeshMesh
AppMeshRoute
AppMeshVirtualGateway
AppMeshVirtualNode
AppMeshVirtualRouter
AppMeshVirtualService
AppStreamDirectoryConfig
AppStreamFleet
AppStreamFleetState
AppStreamImage
AppStreamImageBuilder
AppStreamImageBuilderWaiter
AppStreamStack
AppStreamStackFleetAttachment
AppSyncGraphqlAPI
ApplicationAutoScalingScalableTarget
ArchiveRule
AthenaNamedQuery
AthenaWorkGroup
AutoScalingGroup
AutoScalingPlansScalingPlan
BatchComputeEnvironment
BatchComputeEnvironmentState
BatchJobQueue
BatchJobQueueState
BillingCostandUsageReport
Budget
Cloud9Environment
CloudDirectoryDirectory
CloudDirectorySchema
CloudFormationStack
CloudFormationStackSet
CloudFormationType
CloudFrontDistribution
CloudFrontDistributionDeployment
CloudFrontOriginAccessIdentity
CloudHSMV2Cluster
CloudHSMV2ClusterHSM
CloudSearchDomain
CloudTrailTrail
CloudWatchAlarm
CloudWatchDashboard
CloudWatchEventsBuses
CloudWatchEventsRule
CloudWatchEventsTarget
CloudWatchLogsDestination
CloudWatchLogsLogGroup
CloudWatchLogsResourcePolicy
CodeArtifactDomain
CodeArtifactRepository
CodeBuildProject
CodeCommitRepository
CodeDeployApplication
CodePipelinePipeline
CodeStarConnection
CodeStarNotificationRule
CodeStarProject
CognitoIdentityPool
CognitoIdentityProvider
CognitoUserPool
CognitoUserPoolClient
CognitoUserPoolDomain
ComprehendDocumentClassifier
ComprehendDominantLanguageDetectionJob
ComprehendEndpoint
ComprehendEntitiesDetectionJob
ComprehendEntityRecognizer
ComprehendKeyPhrasesDetectionJob
ComprehendSentimentDetectionJob
ConfigServiceConfigRule
ConfigServiceConfigurationRecorder
ConfigServiceDeliveryChannel
DAXCluster
DAXParameterGroup
DAXSubnetGroup
DataPipelinePipeline
DatabaseMigrationServiceCertificate
DatabaseMigrationServiceEndpoint
DatabaseMigrationServiceEventSubscription
DatabaseMigrationServiceReplicationInstance
DatabaseMigrationServiceReplicationTask
DatabaseMigrationServiceSubnetGroup
DeviceFarmProject
DirectoryServiceDirectory
DynamoDBTable
DynamoDBTableItem
EC2Address
EC2ClientVpnEndpoint
EC2ClientVpnEndpointAttachment
EC2CustomerGateway
EC2DHCPOption
EC2DefaultSecurityGroupRule
EC2EgressOnlyInternetGateway
EC2Host
EC2Image
EC2Instance
EC2InternetGateway
EC2InternetGatewayAttachment
EC2KeyPair
EC2LaunchTemplate
EC2NATGateway
EC2NetworkACL
EC2NetworkInterface
EC2PlacementGroup
EC2RouteTable
EC2SecurityGroup
EC2Snapshot
EC2SpotFleetRequest
EC2Subnet
EC2TGW
EC2TGWAttachment
EC2VPC
EC2VPCEndpoint
EC2VPCEndpointConnection
EC2VPCEndpointServiceConfiguration
EC2VPCPeeringConnection
EC2VPNConnection
EC2VPNGateway
EC2VPNGatewayAttachment
EC2Volume
ECRRepository
ECSCluster
ECSClusterInstance
ECSService
ECSTask
ECSTaskDefinition
EFSFileSystem
EFSMountTarget
EKSCluster
EKSFargateProfiles
EKSNodegroups
ELB
ELBv2
ELBv2TargetGroup
EMRCluster
EMRSecurityConfiguration
ESDomain
ElasticBeanstalkApplication
ElasticBeanstalkEnvironment
ElasticTranscoderPipeline
ElasticacheCacheCluster
ElasticacheCacheParameterGroup
ElasticacheReplicationGroup
ElasticacheSubnetGroup
FMSNotificationChannel
FMSPolicy
FSxBackup
FSxFileSystem
FirehoseDeliveryStream
GlobalAccelerator
GlobalAcceleratorEndpointGroup
GlobalAcceleratorListener
GlueClassifier
GlueConnection
GlueCrawler
GlueDataBrewDatasets
GlueDataBrewJobs
GlueDataBrewProjects
GlueDataBrewRecipe
GlueDataBrewRulesets
GlueDataBrewSchedules
GlueDatabase
GlueDevEndpoint
GlueJob
GlueTrigger
GuardDutyDetector
IAMGroup
IAMGroupPolicy
IAMGroupPolicyAttachment
IAMInstanceProfile
IAMInstanceProfileRole
IAMLoginProfile
IAMOpenIDConnectProvider
IAMPolicy
IAMRole
IAMRolePolicy
IAMRolePolicyAttachment
IAMSAMLProvider
IAMServerCertificate
IAMServiceSpecificCredential
IAMSigningCertificate
IAMUser
IAMUserAccessKey
IAMUserGroupAttachment
IAMUserPolicy
IAMUserPolicyAttachment
IAMUserSSHPublicKey
IAMVirtualMFADevice
ImageBuilderComponent
ImageBuilderDistributionConfiguration
ImageBuilderImage
ImageBuilderInfrastructureConfiguration
ImageBuilderPipeline
ImageBuilderRecipe
Inspector2
InspectorAssessmentRun
InspectorAssessmentTarget
InspectorAssessmentTemplate
IoTAuthorizer
IoTCACertificate
IoTCertificate
IoTJob
IoTOTAUpdate
IoTPolicy
IoTRoleAlias
IoTStream
IoTThing
IoTThingGroup
IoTThingType
IoTThingTypeState
IoTTopicRule
KMSAlias
KMSKey
KendraIndex
KinesisAnalyticsApplication
KinesisStream
KinesisVideoProject
LambdaEventSourceMapping
LambdaFunction
LambdaLayer
LaunchConfiguration
LexBot
LexIntent
LexModelBuildingServiceBotAlias
LexSlotType
LifecycleHook
LightsailDisk
LightsailDomain
LightsailInstance
LightsailKeyPair
LightsailLoadBalancer
LightsailStaticIP
MQBroker
MSKCluster
MSKConfiguration
MachineLearningBranchPrediction
MachineLearningDataSource
MachineLearningEvaluation
MachineLearningMLModel
Macie
MediaConvertJobTemplate
MediaConvertPreset
MediaConvertQueue
MediaLiveChannel
MediaLiveInput
MediaLiveInputSecurityGroup
MediaPackageChannel
MediaPackageOriginEndpoint
MediaStoreContainer
MediaStoreDataItems
MediaTailorConfiguration
MobileProject
NeptuneCluster
NeptuneInstance
NetpuneSnapshot
OSDomain
OpsWorksApp
OpsWorksCMBackup
OpsWorksCMServer
OpsWorksCMServerState
OpsWorksInstance
OpsWorksLayer
OpsWorksUserProfile
QLDBLedger
RDSClusterSnapshot
RDSDBCluster
RDSDBClusterParameterGroup
RDSDBParameterGroup
RDSDBSubnetGroup
RDSEventSubscription
RDSInstance
RDSOptionGroup
RDSProxy
RDSSnapshot
RedshiftCluster
RedshiftParameterGroup
RedshiftSnapshot
RedshiftSubnetGroup
RekognitionCollection
ResourceGroupGroup
RoboMakerRobotApplication
RoboMakerSimulationApplication
RoboMakerSimulationJob
Route53HealthCheck
Route53HostedZone
Route53ResolverEndpoint
Route53ResolverRule
Route53ResourceRecordSet
Route53TrafficPolicy
S3AccessPoint
S3Bucket
S3MultipartUpload
S3Object
SESConfigurationSet
SESIdentity
SESReceiptFilter
SESReceiptRuleSet
SESTemplate
SFNStateMachine
SNSEndpoint
SNSPlatformApplication
SNSSubscription
SNSTopic
SQSQueue
SSMActivation
SSMAssociation
SSMDocument
SSMMaintenanceWindow
SSMParameter
SSMPatchBaseline
SSMResourceDataSync
SageMakerApp
SageMakerDomain
SageMakerEndpoint
SageMakerEndpointConfig
SageMakerModel
SageMakerNotebookInstance
SageMakerNotebookInstanceLifecycleConfig
SageMakerNotebookInstanceState
SageMakerUserProfiles
SecretsManagerSecret
SecurityHub
ServiceCatalogConstraintPortfolioAttachment
ServiceCatalogPortfolio
ServiceCatalogPortfolioProductAttachment
ServiceCatalogPortfolioShareAttachment
ServiceCatalogPrincipalPortfolioAttachment
ServiceCatalogProduct
ServiceCatalogProvisionedProduct
ServiceCatalogTagOption
ServiceCatalogTagOptionPortfolioAttachment
ServiceDiscoveryInstance
ServiceDiscoveryNamespace
ServiceDiscoveryService
SignerSigningJob
SimpleDBDomain
StorageGatewayFileShare
StorageGatewayGateway
StorageGatewayTape
StorageGatewayVolume
TransferServer
TransferServerUser
WAFRegionalByteMatchSet
WAFRegionalByteMatchSetIP
WAFRegionalIPSet
WAFRegionalIPSetIP
WAFRegionalRateBasedRule
WAFRegionalRateBasedRulePredicate
WAFRegionalRegexMatchSet
WAFRegionalRegexMatchTuple
WAFRegionalRegexPatternSet
WAFRegionalRegexPatternString
WAFRegionalRule
WAFRegionalRuleGroup
WAFRegionalRulePredicate
WAFRegionalWebACL
WAFRegionalWebACLRuleAttachment
WAFRule
WAFWebACL
WAFWebACLRuleAttachment
WAFv2IPSet
WAFv2RegexPatternSet
WAFv2RuleGroup
WAFv2WebACL
WorkSpacesWorkspace
XRayGroup
XRaySamplingRule"""
    eng_list = [t.strip() for t in raw_list.splitlines() if t.strip()]
    priorities = {"EC2Instance", "IAMUser", "LambdaFunction", "RDSInstance", "S3Bucket", "EC2VPC"}
    p_items = sorted([t for t in eng_list if t in priorities])
    other_items = sorted([t for t in eng_list if t not in priorities])
    final_eng = ["<모두>"] + p_items + other_items
    with open("f:/aws-nuke/cpp_doom/resource_list.h", "w", encoding="utf-8") as out:
        out.write("#ifndef RESOURCE_LIST_H\n#define RESOURCE_LIST_H\n\n")
        out.write("struct ResourceInfo {\n")
        out.write("    const wchar_t* eng;\n")
        out.write("    const wchar_t* kor;\n")
        out.write("    const wchar_t* tags;\n")
        out.write("};\n\n")
        out.write("const ResourceInfo g_resourceInfos[] = {\n")
        for eng in final_eng:
            k = translate(eng)
            t = get_tags(eng, k)
            out.write(f'    {{ L"{eng}", L"{k}", L"{t}" }},\n')
        out.write("};\n")
        out.write("const int g_numResourceTypes = sizeof(g_resourceInfos) / sizeof(g_resourceInfos[0]);\n\n")
        out.write("#endif\n")
    print(f"Generated resource_list.h with {len(final_eng)} items.")

if __name__ == "__main__":
    generate_header()
