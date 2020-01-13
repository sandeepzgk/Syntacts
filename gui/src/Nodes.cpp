#include "Nodes.hpp"
#include "Gui.hpp"

using namespace carnot;

namespace ImGui {

namespace
{
bool g_nodeHeld = false;
PItem g_pPayload;
std::string g_lPayload;
float g_nodeTime;
}

void SetNodeHeld(bool held)
{
    g_nodeHeld = held;
    if (held)
        g_nodeTime = 0;
}

bool NodeHeld() {
    return g_nodeHeld;
}

void NodeSlot(const char* label, const ImVec2& size, ImGuiCol col)
{
    BeginNodeTarget(col);   
    ImGui::Button(label, size);
    EndNodeTarget();
}

void BeginNodeTarget(ImGuiCol col) {
    ImVec4 color = ImGui::GetStyle().Colors[col];
    if (g_nodeHeld)
    {        
        static float f = 1.0f;
        float t = 0.5f + 0.5f * std::sin(2.0f * IM_PI * f * (float)GetTime());
        color = ImLerp(color, ImVec4(0,0,0,0), t * 0.25f);
    }
    ImGui::PushStyleColor(ImGuiCol_Button, color);   
    ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
}

void EndNodeTarget() {
    ImGui::PopStyleColor(2);
}

void NodeSourceP(PItem pItem) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        g_pPayload = pItem;
        SetNodeHeld(true);
        ImGui::SetDragDropPayload("DND_PITEM", &g_pPayload, sizeof(PItem));
        ImGui::Text(palletteString(pItem).c_str());
        ImGui::EndDragDropSource();
    }  
}

void NodeSourceL(const std::string& name) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        g_lPayload = name;
        SetNodeHeld(true);
        ImGui::SetDragDropPayload("DND_LITEM", &g_lPayload, sizeof(std::string));
        ImGui::Text(name.c_str());
        ImGui::EndDragDropSource();
    }
}

bool NodeDroppedP()
{
    bool ret = false;
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DND_PITEM"))
        {
            ret = true;
        }
        ImGui::EndDragDropTarget();
    }
    return ret;
}

bool NodeDroppedL() {
    bool ret = false;
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DND_LITEM"))
        {
            ret = true;
        }
        ImGui::EndDragDropTarget();
    }
    return ret;
}

PItem NodePayloadP() {
    return g_pPayload;
}

const std::string& NodePayloadL() {
    return g_lPayload;
}

void NodeUpdate() {
    SetNodeHeld(false);
    g_nodeTime = Engine::deltaTime();
}

}

///////////////////////////////////////////////////////////////////////////////

/// Returns the name of a Syntacts signal
const std::string &signalName(std::type_index id)
{
    static std::string unkown = "Unkown";
    static std::unordered_map<std::type_index, std::string> names = {
        // {typeid(tact::Zero),           "Zero"},
        {typeid(tact::Time), "Time"},
        {typeid(tact::Scalar), "Scalar"},
        {typeid(tact::Ramp), "Ramp"},
        {typeid(tact::Noise), "Noise"},
        {typeid(tact::Expression), "Expression"},
        {typeid(tact::Sum), "Sum"},
        {typeid(tact::Product), "Product"},
        {typeid(tact::Sine), "Sine"},
        {typeid(tact::Square), "Square"},
        {typeid(tact::Saw), "Saw"},
        {typeid(tact::Triangle), "Triangle"},
        {typeid(tact::Chirp), "Chirp"},
        {typeid(tact::Pwm), "PWM"},
        {typeid(tact::Envelope), "Envelope"},
        {typeid(tact::ASR), "ASR"},
        {typeid(tact::ADSR), "ADSR"},
        {typeid(tact::KeyedEnvelope), "Keyed Envelope"},
        {typeid(tact::SignalEnvelope), "Signal Envelope"},
        {typeid(tact::PolyBezier), "PolyBezier"}};
    if (names.count(id))
        return names[id];
    else
        return unkown;
}

const std::string &signalName(const tact::Signal &sig)
{
    return signalName(sig.typeId());
}

///////////////////////////////////////////////////////////////////////////////

void NodeList::gui()
{
    // render nodes
    for (std::size_t i = 0; i < m_nodes.size(); ++i)
    {
        auto &header = m_nodes[i]->name() + "###SignalSlot";
        ImGui::PushID(m_ids[i]);
        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_TabActive]);
        if (ImGui::CollapsingHeader(header.c_str(), &m_closeHandles[i]))
        {
            ImGui::Indent();
            m_nodes[i]->gui();
            ImGui::Unindent();
        }
        ImGui::PopStyleColor();
        ImGui::PopID();
    }
    // node slot
    if (m_nodes.size() == 0 || ImGui::NodeHeld())
        ImGui::NodeSlot("##EmpySlot",ImVec2(-1,0),ImGuiCol_TabActive);
    // check for incomming palette items
    if (ImGui::NodeDroppedP())
    {
        auto node = makeNode(ImGui::NodePayloadP());
        if (node) {
            m_nodes.emplace_back(node);
            m_closeHandles.emplace_back(true);
            m_ids.push_back(m_nextId++);
        }
    }
    // check for incomming library items
    if (ImGui::NodeDroppedL()) {
        auto node = make<LibrarySignalNode>(ImGui::NodePayloadL());
        m_nodes.emplace_back(node);
        m_closeHandles.emplace_back(true);
        m_ids.push_back(m_nextId++);
    }
    // clean up
    std::vector<Ptr<Node>> newNodes;
    std::vector<int> newIds;
    for (int i = 0; i < m_nodes.size(); ++i)
    {
        if (m_closeHandles[i])
        {
            newNodes.emplace_back(std::move(m_nodes[i]));
            newIds.push_back(m_ids[i]);
        }
    }
    m_nodes = std::move(newNodes);
    m_ids = std::move(newIds);
    m_closeHandles = std::deque<bool>(m_nodes.size(), true);
}


///////////////////////////////////////////////////////////////////////////////

tact::Signal ProductNode::signal()
{
    if (m_nodes.size() > 0)
    {
        auto sig = m_nodes[0]->signal();
        for (int i = 1; i < m_nodes.size(); ++i)
            sig = sig * m_nodes[i]->signal();
        return sig;
    }
    return tact::Signal();
}

const std::string& ProductNode::name()
{
    static std::string n = "Product";
    return n;
}

///////////////////////////////////////////////////////////////////////////////

tact::Signal SumNode::signal()
{
    if (m_nodes.size() > 0)
    {
        auto sig = m_nodes[0]->signal();
        for (int i = 1; i < m_nodes.size(); ++i)
            sig = sig + m_nodes[i]->signal();
        return sig;
    }
    return tact::Signal();
}

const std::string& SumNode::name()
{
    static std::string n = "Sum";
    return n;
}

///////////////////////////////////////////////////////////////////////////////

LibrarySignalNode::LibrarySignalNode(const std::string& name) : libName(name) {
    tact::Library::loadSignal(sig, libName);
}

tact::Signal LibrarySignalNode::signal() { return sig; }
const std::string& LibrarySignalNode::name() { return libName; }
// float* LibrarySignalNode::gain() { return &sig.scale; }
// float* LibrarySignalNode::bias() { return &sig.offset; }
void LibrarySignalNode::gui() {

}


///////////////////////////////////////////////////////////////////////////////

OscillatorNode::OscillatorNode(tact::Signal osc)
{
    sig = std::move(osc);
}

void OscillatorNode::gui()
{
    auto cast = (tact::IOscillator *)sig.get();
    if (ImGui::RadioButton("Sine", sig.isType<tact::Sine>()))
    {
        sig = tact::Sine(cast->x);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Square", sig.isType<tact::Square>()))
    {
        sig = tact::Square(cast->x);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Saw", sig.isType<tact::Saw>()))
    {
        sig = tact::Saw(cast->x);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Triangle", sig.isType<tact::Triangle>()))
    {
        sig = tact::Triangle(cast->x);
    }
    cast = (tact::IOscillator *)sig.get();
    if (cast->x.isType<tact::Time>())
    {
        float f = (float)cast->x.gain / (float)tact::TWO_PI;
        ImGui::BeginNodeTarget();
        ImGui::DragFloat("##Frequency", &f, 1, 0, 1000, "%.0f Hz");
        ImGui::EndNodeTarget();
        if (ImGui::NodeDroppedP()) {

        }
        ImGui::SameLine(); ImGui::Text("Frequency");
        cast->x.gain = f * tact::TWO_PI;
    }
    else
    {
        ImGui::Indent();
        ImGui::Text("Oops!");
        ImGui::Unindent();
    }
}

///////////////////////////////////////////////////////////////////////////////

void ChirpNode::gui()
{
    auto cast = (tact::Chirp *)sig.get();
    if (cast->x.isType<tact::Time>())
    {
        float f = (float)cast->x.gain / (float)tact::TWO_PI;
        ImGui::DragFloat("Frequency", &f, 1, 0, 1000, "%.0f Hz");
        cast->x.gain = f * tact::TWO_PI;
    }
    float rate = (float)cast->rate;
    ImGui::DragFloat("Rate", &rate, 1, 0, 0, "%.0f Hz/s");
    cast->rate = rate;
}

///////////////////////////////////////////////////////////////////////////////

ExpressionNode::ExpressionNode()
{
    auto cast = (tact::Expression *)sig.get();
    strcpy(buffer, cast->getExpression().c_str());
}

void ExpressionNode::gui()
{
    if (!ok)
        ImGui::PushStyleColor(ImGuiCol_FrameBg, Reds::FireBrick);
    bool entered = ImGui::InputText("f(t)", buffer, 256, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
    if (!ok)
        ImGui::PopStyleColor();
    if (entered)
    {
        auto cast = (tact::Expression *)sig.get();
        std::string oldExpr = cast->getExpression();
        std::string newExpr(buffer);
        if (!cast->setExpression(newExpr))
        {
            cast->setExpression(oldExpr);
            ok = false;
        }
        else
        {
            ok = true;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

PolyBezierNode::PolyBezierNode() : pb(colors[(colorIdx++) % colors.size()], ImVec2(0, 0), ImVec2(1, 1)) {}

void PolyBezierNode::gui()
{
    auto cast = (tact::PolyBezier *)sig.get();
    ImGui::PolyBezierEdit("PolyBezier", &pb, 10, 10, ImVec2(-1, 125));
    int points = pb.pointCount();
    cast->points.resize(points);
    for (int i = 0; i < points; ++i)
    {
        ImVec2 cpL, pos, cpR;
        pb.getPoint(i, &cpL, &pos, &cpR);
        cast->points[i].cpL = {cpL.x, cpL.y};
        cast->points[i].p = {pos.x, pos.y};
        cast->points[i].cpR = {cpR.x, cpR.y};
    }
    cast->solve();
}

std::vector<Color> PolyBezierNode::colors = std::vector<Color>({Blues::DeepSkyBlue, Greens::Chartreuse, Reds::FireBrick});
int PolyBezierNode::colorIdx = 0;

///////////////////////////////////////////////////////////////////////////////

void EnvelopeNode::gui()
{
    auto cast = (tact::Envelope*)sig.get();
    float duration  = (float)cast->duration;
    float amplitude = (float)cast->amplitude;
    ImGui::DragFloat("Duration", &duration, 0.001f, 0, 1, "%0.3f s");
    ImGui::DragFloat("Amplitude", &amplitude, 0.01f, 0, 1);
    cast->amplitude = amplitude;
    cast->duration = duration;
}

///////////////////////////////////////////////////////////////////////////////

void ASRNode::gui()
{
    auto cast = (tact::ASR *)sig.get();
    auto it = cast->keys.begin();
    it++;
    auto &a = *(it++);
    auto &s = *(it++);
    auto &r = *(it++);

    float asr[3];
    asr[0]  = a.first;
    asr[1] = s.first - a.first;
    asr[2] = r.first - s.first;

    float amp     = a.second.first;

    bool changed = false;
    if (ImGui::DragFloat3("Durations", asr, 0.001f, 0.0001f, 1.0f, "%0.3f s"))
        changed = true;
    if (ImGui::DragFloat("Amplitude", &amp, 0.001f, 0, 1))
        changed = true;

    if (changed)
        sig = tact::ASR(asr[0], asr[1], asr[2], amp);
}

///////////////////////////////////////////////////////////////////////////////

void ADSRNode::gui()
{
    auto cast = (tact::ASR *)sig.get();
    auto it = cast->keys.begin();
    it++;
    auto &a = *(it++);
    auto &d = *(it++);
    auto &s = *(it++);
    auto &r = *(it++);
    float adsr[4];
    adsr[0] = a.first;
    adsr[1] = d.first - a.first;
    adsr[2] = s.first - d.first;
    adsr[3] = r.first - s.first;
    float amp[2];
    amp[0] = a.second.first;
    amp[1] = d.second.first;
    bool changed = false;
    if (ImGui::DragFloat4("Durations", adsr, 0.001f, 0.0001f, 1, "%0.3f s"))
        changed = true;
    if (ImGui::DragFloat2("Amplitudes", amp, 0.001f, 0, 1))
        changed = true;
    if (changed)
        sig = tact::ADSR(adsr[0], adsr[1], adsr[2], adsr[3], amp[0], amp[1]);
}

///////////////////////////////////////////////////////////////////////////////

void NoiseNode::gui()
{
    float gain = (float)sig.gain;
    float bias = (float)sig.bias;
    ImGui::DragFloat("Gain", &gain, 0.001f, -1, 1, "%0.3f");
    ImGui::DragFloat("Bias", &bias, 0.001f, -1, 1, "%0.3f");
    sig.gain = gain; sig.bias = bias;
}

///////////////////////////////////////////////////////////////////////////////

void TimeNode::gui() {}

///////////////////////////////////////////////////////////////////////////////

void ScalarNode::gui()
{
    auto cast = (tact::Scalar *)sig.get();
    float value = (float)cast->value;
    ImGui::DragFloat("Value", &value, 0.01f, -10, 10);
    cast->value = value;
}

///////////////////////////////////////////////////////////////////////////////

void RampNode::gui()
{
    auto cast = (tact::Ramp *)sig.get();
    float initial = (float)cast->initial;
    float rate = (float)cast->rate;
    ImGui::DragFloat("Initial", &initial, 0.01f, -10, 10);
    ImGui::DragFloat("Rate", &rate, 0.01f, -10, 10);
    cast->initial = initial;
    cast->rate = rate;
}

///////////////////////////////////////////////////////////////////////////////

Ptr<Node> makeNode(PItem id)
{
    if (id == PItem::Time)
        return make<TimeNode>();
    if (id == PItem::Scalar)
        return make<ScalarNode>();
    if (id == PItem::Ramp)
        return make<RampNode>();
    if (id == PItem::Noise)
        return make<NoiseNode>();
    if (id == PItem::Expression)
        return make<ExpressionNode>();
    if (id == PItem::Sum)
        return make<SumNode>();
    if (id == PItem::Product)
        return make<ProductNode>();
    if (id == PItem::Sine)
        return make<OscillatorNode>(tact::Sine());
    if (id == PItem::Square)
        return make<OscillatorNode>(tact::Square());
    if (id == PItem::Saw)
        return make<OscillatorNode>(tact::Saw());
    if (id == PItem::Triangle)
        return make<OscillatorNode>(tact::Triangle());
    if (id == PItem::Chirp)
        return make<ChirpNode>();
    if (id == PItem::Envelope)
        return make<EnvelopeNode>();
    if (id == PItem::ASR)
        return make<ASRNode>();
    if (id == PItem::ADSR)
        return make<ADSRNode>();
    if (id == PItem::PolyBezier)
        return make<PolyBezierNode>();
    static auto gui = Engine::getRoot().as<Gui>();
    gui->status->pushMessage("Failed to create Node!", StatusBar::Error);
    return nullptr;
}