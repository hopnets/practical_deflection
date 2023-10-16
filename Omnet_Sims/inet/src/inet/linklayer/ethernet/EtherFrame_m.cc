//
// Generated file, do not edit! Created by nedtool 5.6 from inet/linklayer/ethernet/EtherFrame.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include <memory>
#include "EtherFrame_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp

namespace {
template <class T> inline
typename std::enable_if<std::is_polymorphic<T>::value && std::is_base_of<omnetpp::cObject,T>::value, void *>::type
toVoidPtr(T* t)
{
    return (void *)(static_cast<const omnetpp::cObject *>(t));
}

template <class T> inline
typename std::enable_if<std::is_polymorphic<T>::value && !std::is_base_of<omnetpp::cObject,T>::value, void *>::type
toVoidPtr(T* t)
{
    return (void *)dynamic_cast<const void *>(t);
}

template <class T> inline
typename std::enable_if<!std::is_polymorphic<T>::value, void *>::type
toVoidPtr(T* t)
{
    return (void *)static_cast<const void *>(t);
}

}

namespace inet {

// forward
template<typename T, typename A>
std::ostream& operator<<(std::ostream& out, const std::vector<T,A>& vec);

// Template rule to generate operator<< for shared_ptr<T>
template<typename T>
inline std::ostream& operator<<(std::ostream& out,const std::shared_ptr<T>& t) { return out << t.get(); }

// Template rule which fires if a struct or class doesn't have operator<<
template<typename T>
inline std::ostream& operator<<(std::ostream& out,const T&) {return out;}

// operator<< for std::vector<T>
template<typename T, typename A>
inline std::ostream& operator<<(std::ostream& out, const std::vector<T,A>& vec)
{
    out.put('{');
    for(typename std::vector<T,A>::const_iterator it = vec.begin(); it != vec.end(); ++it)
    {
        if (it != vec.begin()) {
            out.put(','); out.put(' ');
        }
        out << *it;
    }
    out.put('}');

    char buf[32];
    sprintf(buf, " (size=%u)", (unsigned int)vec.size());
    out.write(buf, strlen(buf));
    return out;
}

EXECUTE_ON_STARTUP(
    omnetpp::cEnum *e = omnetpp::cEnum::find("inet::EthernetControlOpCode");
    if (!e) omnetpp::enums.getInstance()->add(e = new omnetpp::cEnum("inet::EthernetControlOpCode"));
    e->insert(ETHERNET_CONTROL_PAUSE, "ETHERNET_CONTROL_PAUSE");
)

Register_Class(EthernetMacHeader)

EthernetMacHeader::EthernetMacHeader() : ::inet::FieldsChunk()
{
    this->setChunkLength(B(ETHER_MAC_HEADER_BYTES));

}

EthernetMacHeader::EthernetMacHeader(const EthernetMacHeader& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

EthernetMacHeader::~EthernetMacHeader()
{
    delete this->cTag;
    delete this->sTag;
}

EthernetMacHeader& EthernetMacHeader::operator=(const EthernetMacHeader& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void EthernetMacHeader::copy(const EthernetMacHeader& other)
{
    this->dest = other.dest;
    this->src = other.src;
    delete this->cTag;
    this->cTag = other.cTag;
    if (this->cTag != nullptr) {
        this->cTag = this->cTag->dup();
    }
    delete this->sTag;
    this->sTag = other.sTag;
    if (this->sTag != nullptr) {
        this->sTag = this->sTag->dup();
    }
    this->typeOrLength = other.typeOrLength;
    this->hop_count = other.hop_count;
    this->isFB_ = other.isFB_;
    this->allow_same_input_output = other.allow_same_input_output;
    this->original_interface_id = other.original_interface_id;
    this->bouncedDistance = other.bouncedDistance;
    this->maxBouncedDistance = other.maxBouncedDistance;
    this->bouncedHop = other.bouncedHop;
    this->totalHopNum = other.totalHopNum;
    this->is_bursty = other.is_bursty;
    this->payload_length = other.payload_length;
    this->total_length = other.total_length;
    this->offset = other.offset;
    this->is_v2_dropped_packet_header = other.is_v2_dropped_packet_header;
    this->queue_occupancy = other.queue_occupancy;
    this->time_packet_received_at_nic = other.time_packet_received_at_nic;
    this->local_nic_rx_delay = other.local_nic_rx_delay;
    this->remote_queueing_time = other.remote_queueing_time;
    this->fabric_delay_time_sent_from_source = other.fabric_delay_time_sent_from_source;
    this->time_packet_sent_from_src = other.time_packet_sent_from_src;
    this->is_deflected = other.is_deflected;
}

void EthernetMacHeader::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
    doParsimPacking(b,this->dest);
    doParsimPacking(b,this->src);
    doParsimPacking(b,this->cTag);
    doParsimPacking(b,this->sTag);
    doParsimPacking(b,this->typeOrLength);
    doParsimPacking(b,this->hop_count);
    doParsimPacking(b,this->isFB_);
    doParsimPacking(b,this->allow_same_input_output);
    doParsimPacking(b,this->original_interface_id);
    doParsimPacking(b,this->bouncedDistance);
    doParsimPacking(b,this->maxBouncedDistance);
    doParsimPacking(b,this->bouncedHop);
    doParsimPacking(b,this->totalHopNum);
    doParsimPacking(b,this->is_bursty);
    doParsimPacking(b,this->payload_length);
    doParsimPacking(b,this->total_length);
    doParsimPacking(b,this->offset);
    doParsimPacking(b,this->is_v2_dropped_packet_header);
    doParsimPacking(b,this->queue_occupancy);
    doParsimPacking(b,this->time_packet_received_at_nic);
    doParsimPacking(b,this->local_nic_rx_delay);
    doParsimPacking(b,this->remote_queueing_time);
    doParsimPacking(b,this->fabric_delay_time_sent_from_source);
    doParsimPacking(b,this->time_packet_sent_from_src);
    doParsimPacking(b,this->is_deflected);
}

void EthernetMacHeader::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
    doParsimUnpacking(b,this->dest);
    doParsimUnpacking(b,this->src);
    doParsimUnpacking(b,this->cTag);
    doParsimUnpacking(b,this->sTag);
    doParsimUnpacking(b,this->typeOrLength);
    doParsimUnpacking(b,this->hop_count);
    doParsimUnpacking(b,this->isFB_);
    doParsimUnpacking(b,this->allow_same_input_output);
    doParsimUnpacking(b,this->original_interface_id);
    doParsimUnpacking(b,this->bouncedDistance);
    doParsimUnpacking(b,this->maxBouncedDistance);
    doParsimUnpacking(b,this->bouncedHop);
    doParsimUnpacking(b,this->totalHopNum);
    doParsimUnpacking(b,this->is_bursty);
    doParsimUnpacking(b,this->payload_length);
    doParsimUnpacking(b,this->total_length);
    doParsimUnpacking(b,this->offset);
    doParsimUnpacking(b,this->is_v2_dropped_packet_header);
    doParsimUnpacking(b,this->queue_occupancy);
    doParsimUnpacking(b,this->time_packet_received_at_nic);
    doParsimUnpacking(b,this->local_nic_rx_delay);
    doParsimUnpacking(b,this->remote_queueing_time);
    doParsimUnpacking(b,this->fabric_delay_time_sent_from_source);
    doParsimUnpacking(b,this->time_packet_sent_from_src);
    doParsimUnpacking(b,this->is_deflected);
}

const MacAddress& EthernetMacHeader::getDest() const
{
    return this->dest;
}

void EthernetMacHeader::setDest(const MacAddress& dest)
{
    handleChange();
    this->dest = dest;
}

const MacAddress& EthernetMacHeader::getSrc() const
{
    return this->src;
}

void EthernetMacHeader::setSrc(const MacAddress& src)
{
    handleChange();
    this->src = src;
}

const Ieee8021qHeader * EthernetMacHeader::getCTag() const
{
    return this->cTag;
}

void EthernetMacHeader::setCTag(Ieee8021qHeader * cTag)
{
    handleChange();
    if (this->cTag != nullptr) throw omnetpp::cRuntimeError("setCTag(): a value is already set, remove it first with dropCTag()");
    this->cTag = cTag;
}

Ieee8021qHeader * EthernetMacHeader::dropCTag()
{
    handleChange();
    Ieee8021qHeader * retval = this->cTag;
    this->cTag = nullptr;
    return retval;
}

const Ieee8021qHeader * EthernetMacHeader::getSTag() const
{
    return this->sTag;
}

void EthernetMacHeader::setSTag(Ieee8021qHeader * sTag)
{
    handleChange();
    if (this->sTag != nullptr) throw omnetpp::cRuntimeError("setSTag(): a value is already set, remove it first with dropSTag()");
    this->sTag = sTag;
}

Ieee8021qHeader * EthernetMacHeader::dropSTag()
{
    handleChange();
    Ieee8021qHeader * retval = this->sTag;
    this->sTag = nullptr;
    return retval;
}

int EthernetMacHeader::getTypeOrLength() const
{
    return this->typeOrLength;
}

void EthernetMacHeader::setTypeOrLength(int typeOrLength)
{
    handleChange();
    this->typeOrLength = typeOrLength;
}

int EthernetMacHeader::getHop_count() const
{
    return this->hop_count;
}

void EthernetMacHeader::setHop_count(int hop_count)
{
    handleChange();
    this->hop_count = hop_count;
}

bool EthernetMacHeader::isFB() const
{
    return this->isFB_;
}

void EthernetMacHeader::setIsFB(bool isFB)
{
    handleChange();
    this->isFB_ = isFB;
}

bool EthernetMacHeader::getAllow_same_input_output() const
{
    return this->allow_same_input_output;
}

void EthernetMacHeader::setAllow_same_input_output(bool allow_same_input_output)
{
    handleChange();
    this->allow_same_input_output = allow_same_input_output;
}

int EthernetMacHeader::getOriginal_interface_id() const
{
    return this->original_interface_id;
}

void EthernetMacHeader::setOriginal_interface_id(int original_interface_id)
{
    handleChange();
    this->original_interface_id = original_interface_id;
}

int EthernetMacHeader::getBouncedDistance() const
{
    return this->bouncedDistance;
}

void EthernetMacHeader::setBouncedDistance(int bouncedDistance)
{
    handleChange();
    this->bouncedDistance = bouncedDistance;
}

int EthernetMacHeader::getMaxBouncedDistance() const
{
    return this->maxBouncedDistance;
}

void EthernetMacHeader::setMaxBouncedDistance(int maxBouncedDistance)
{
    handleChange();
    this->maxBouncedDistance = maxBouncedDistance;
}

int EthernetMacHeader::getBouncedHop() const
{
    return this->bouncedHop;
}

void EthernetMacHeader::setBouncedHop(int bouncedHop)
{
    handleChange();
    this->bouncedHop = bouncedHop;
}

int EthernetMacHeader::getTotalHopNum() const
{
    return this->totalHopNum;
}

void EthernetMacHeader::setTotalHopNum(int totalHopNum)
{
    handleChange();
    this->totalHopNum = totalHopNum;
}

bool EthernetMacHeader::getIs_bursty() const
{
    return this->is_bursty;
}

void EthernetMacHeader::setIs_bursty(bool is_bursty)
{
    handleChange();
    this->is_bursty = is_bursty;
}

b EthernetMacHeader::getPayload_length() const
{
    return this->payload_length;
}

void EthernetMacHeader::setPayload_length(b payload_length)
{
    handleChange();
    this->payload_length = payload_length;
}

b EthernetMacHeader::getTotal_length() const
{
    return this->total_length;
}

void EthernetMacHeader::setTotal_length(b total_length)
{
    handleChange();
    this->total_length = total_length;
}

b EthernetMacHeader::getOffset() const
{
    return this->offset;
}

void EthernetMacHeader::setOffset(b offset)
{
    handleChange();
    this->offset = offset;
}

bool EthernetMacHeader::getIs_v2_dropped_packet_header() const
{
    return this->is_v2_dropped_packet_header;
}

void EthernetMacHeader::setIs_v2_dropped_packet_header(bool is_v2_dropped_packet_header)
{
    handleChange();
    this->is_v2_dropped_packet_header = is_v2_dropped_packet_header;
}

uint16_t EthernetMacHeader::getQueue_occupancy() const
{
    return this->queue_occupancy;
}

void EthernetMacHeader::setQueue_occupancy(uint16_t queue_occupancy)
{
    handleChange();
    this->queue_occupancy = queue_occupancy;
}

omnetpp::simtime_t EthernetMacHeader::getTime_packet_received_at_nic() const
{
    return this->time_packet_received_at_nic;
}

void EthernetMacHeader::setTime_packet_received_at_nic(omnetpp::simtime_t time_packet_received_at_nic)
{
    handleChange();
    this->time_packet_received_at_nic = time_packet_received_at_nic;
}

omnetpp::simtime_t EthernetMacHeader::getLocal_nic_rx_delay() const
{
    return this->local_nic_rx_delay;
}

void EthernetMacHeader::setLocal_nic_rx_delay(omnetpp::simtime_t local_nic_rx_delay)
{
    handleChange();
    this->local_nic_rx_delay = local_nic_rx_delay;
}

omnetpp::simtime_t EthernetMacHeader::getRemote_queueing_time() const
{
    return this->remote_queueing_time;
}

void EthernetMacHeader::setRemote_queueing_time(omnetpp::simtime_t remote_queueing_time)
{
    handleChange();
    this->remote_queueing_time = remote_queueing_time;
}

omnetpp::simtime_t EthernetMacHeader::getFabric_delay_time_sent_from_source() const
{
    return this->fabric_delay_time_sent_from_source;
}

void EthernetMacHeader::setFabric_delay_time_sent_from_source(omnetpp::simtime_t fabric_delay_time_sent_from_source)
{
    handleChange();
    this->fabric_delay_time_sent_from_source = fabric_delay_time_sent_from_source;
}

omnetpp::simtime_t EthernetMacHeader::getTime_packet_sent_from_src() const
{
    return this->time_packet_sent_from_src;
}

void EthernetMacHeader::setTime_packet_sent_from_src(omnetpp::simtime_t time_packet_sent_from_src)
{
    handleChange();
    this->time_packet_sent_from_src = time_packet_sent_from_src;
}

bool EthernetMacHeader::getIs_deflected() const
{
    return this->is_deflected;
}

void EthernetMacHeader::setIs_deflected(bool is_deflected)
{
    handleChange();
    this->is_deflected = is_deflected;
}

class EthernetMacHeaderDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertynames;
    enum FieldConstants {
        FIELD_dest,
        FIELD_src,
        FIELD_cTag,
        FIELD_sTag,
        FIELD_typeOrLength,
        FIELD_hop_count,
        FIELD_isFB,
        FIELD_allow_same_input_output,
        FIELD_original_interface_id,
        FIELD_bouncedDistance,
        FIELD_maxBouncedDistance,
        FIELD_bouncedHop,
        FIELD_totalHopNum,
        FIELD_is_bursty,
        FIELD_payload_length,
        FIELD_total_length,
        FIELD_offset,
        FIELD_is_v2_dropped_packet_header,
        FIELD_queue_occupancy,
        FIELD_time_packet_received_at_nic,
        FIELD_local_nic_rx_delay,
        FIELD_remote_queueing_time,
        FIELD_fabric_delay_time_sent_from_source,
        FIELD_time_packet_sent_from_src,
        FIELD_is_deflected,
    };
  public:
    EthernetMacHeaderDescriptor();
    virtual ~EthernetMacHeaderDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyname) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyname) const override;
    virtual int getFieldArraySize(void *object, int field) const override;

    virtual const char *getFieldDynamicTypeString(void *object, int field, int i) const override;
    virtual std::string getFieldValueAsString(void *object, int field, int i) const override;
    virtual bool setFieldValueAsString(void *object, int field, int i, const char *value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual void *getFieldStructValuePointer(void *object, int field, int i) const override;
};

Register_ClassDescriptor(EthernetMacHeaderDescriptor)

EthernetMacHeaderDescriptor::EthernetMacHeaderDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(inet::EthernetMacHeader)), "inet::FieldsChunk")
{
    propertynames = nullptr;
}

EthernetMacHeaderDescriptor::~EthernetMacHeaderDescriptor()
{
    delete[] propertynames;
}

bool EthernetMacHeaderDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<EthernetMacHeader *>(obj)!=nullptr;
}

const char **EthernetMacHeaderDescriptor::getPropertyNames() const
{
    if (!propertynames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
        const char **basenames = basedesc ? basedesc->getPropertyNames() : nullptr;
        propertynames = mergeLists(basenames, names);
    }
    return propertynames;
}

const char *EthernetMacHeaderDescriptor::getProperty(const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : nullptr;
}

int EthernetMacHeaderDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 25+basedesc->getFieldCount() : 25;
}

unsigned int EthernetMacHeaderDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeFlags(field);
        field -= basedesc->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        0,    // FIELD_dest
        0,    // FIELD_src
        FD_ISCOMPOUND | FD_ISPOINTER | FD_ISCOBJECT,    // FIELD_cTag
        FD_ISCOMPOUND | FD_ISPOINTER | FD_ISCOBJECT,    // FIELD_sTag
        FD_ISEDITABLE,    // FIELD_typeOrLength
        FD_ISEDITABLE,    // FIELD_hop_count
        FD_ISEDITABLE,    // FIELD_isFB
        FD_ISEDITABLE,    // FIELD_allow_same_input_output
        FD_ISEDITABLE,    // FIELD_original_interface_id
        FD_ISEDITABLE,    // FIELD_bouncedDistance
        FD_ISEDITABLE,    // FIELD_maxBouncedDistance
        FD_ISEDITABLE,    // FIELD_bouncedHop
        FD_ISEDITABLE,    // FIELD_totalHopNum
        FD_ISEDITABLE,    // FIELD_is_bursty
        FD_ISEDITABLE,    // FIELD_payload_length
        FD_ISEDITABLE,    // FIELD_total_length
        FD_ISEDITABLE,    // FIELD_offset
        FD_ISEDITABLE,    // FIELD_is_v2_dropped_packet_header
        FD_ISEDITABLE,    // FIELD_queue_occupancy
        0,    // FIELD_time_packet_received_at_nic
        0,    // FIELD_local_nic_rx_delay
        0,    // FIELD_remote_queueing_time
        0,    // FIELD_fabric_delay_time_sent_from_source
        0,    // FIELD_time_packet_sent_from_src
        FD_ISEDITABLE,    // FIELD_is_deflected
    };
    return (field >= 0 && field < 25) ? fieldTypeFlags[field] : 0;
}

const char *EthernetMacHeaderDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldName(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldNames[] = {
        "dest",
        "src",
        "cTag",
        "sTag",
        "typeOrLength",
        "hop_count",
        "isFB",
        "allow_same_input_output",
        "original_interface_id",
        "bouncedDistance",
        "maxBouncedDistance",
        "bouncedHop",
        "totalHopNum",
        "is_bursty",
        "payload_length",
        "total_length",
        "offset",
        "is_v2_dropped_packet_header",
        "queue_occupancy",
        "time_packet_received_at_nic",
        "local_nic_rx_delay",
        "remote_queueing_time",
        "fabric_delay_time_sent_from_source",
        "time_packet_sent_from_src",
        "is_deflected",
    };
    return (field >= 0 && field < 25) ? fieldNames[field] : nullptr;
}

int EthernetMacHeaderDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount() : 0;
    if (fieldName[0] == 'd' && strcmp(fieldName, "dest") == 0) return base+0;
    if (fieldName[0] == 's' && strcmp(fieldName, "src") == 0) return base+1;
    if (fieldName[0] == 'c' && strcmp(fieldName, "cTag") == 0) return base+2;
    if (fieldName[0] == 's' && strcmp(fieldName, "sTag") == 0) return base+3;
    if (fieldName[0] == 't' && strcmp(fieldName, "typeOrLength") == 0) return base+4;
    if (fieldName[0] == 'h' && strcmp(fieldName, "hop_count") == 0) return base+5;
    if (fieldName[0] == 'i' && strcmp(fieldName, "isFB") == 0) return base+6;
    if (fieldName[0] == 'a' && strcmp(fieldName, "allow_same_input_output") == 0) return base+7;
    if (fieldName[0] == 'o' && strcmp(fieldName, "original_interface_id") == 0) return base+8;
    if (fieldName[0] == 'b' && strcmp(fieldName, "bouncedDistance") == 0) return base+9;
    if (fieldName[0] == 'm' && strcmp(fieldName, "maxBouncedDistance") == 0) return base+10;
    if (fieldName[0] == 'b' && strcmp(fieldName, "bouncedHop") == 0) return base+11;
    if (fieldName[0] == 't' && strcmp(fieldName, "totalHopNum") == 0) return base+12;
    if (fieldName[0] == 'i' && strcmp(fieldName, "is_bursty") == 0) return base+13;
    if (fieldName[0] == 'p' && strcmp(fieldName, "payload_length") == 0) return base+14;
    if (fieldName[0] == 't' && strcmp(fieldName, "total_length") == 0) return base+15;
    if (fieldName[0] == 'o' && strcmp(fieldName, "offset") == 0) return base+16;
    if (fieldName[0] == 'i' && strcmp(fieldName, "is_v2_dropped_packet_header") == 0) return base+17;
    if (fieldName[0] == 'q' && strcmp(fieldName, "queue_occupancy") == 0) return base+18;
    if (fieldName[0] == 't' && strcmp(fieldName, "time_packet_received_at_nic") == 0) return base+19;
    if (fieldName[0] == 'l' && strcmp(fieldName, "local_nic_rx_delay") == 0) return base+20;
    if (fieldName[0] == 'r' && strcmp(fieldName, "remote_queueing_time") == 0) return base+21;
    if (fieldName[0] == 'f' && strcmp(fieldName, "fabric_delay_time_sent_from_source") == 0) return base+22;
    if (fieldName[0] == 't' && strcmp(fieldName, "time_packet_sent_from_src") == 0) return base+23;
    if (fieldName[0] == 'i' && strcmp(fieldName, "is_deflected") == 0) return base+24;
    return basedesc ? basedesc->findField(fieldName) : -1;
}

const char *EthernetMacHeaderDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeString(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "inet::MacAddress",    // FIELD_dest
        "inet::MacAddress",    // FIELD_src
        "inet::Ieee8021qHeader",    // FIELD_cTag
        "inet::Ieee8021qHeader",    // FIELD_sTag
        "int",    // FIELD_typeOrLength
        "int",    // FIELD_hop_count
        "bool",    // FIELD_isFB
        "bool",    // FIELD_allow_same_input_output
        "int",    // FIELD_original_interface_id
        "int",    // FIELD_bouncedDistance
        "int",    // FIELD_maxBouncedDistance
        "int",    // FIELD_bouncedHop
        "int",    // FIELD_totalHopNum
        "bool",    // FIELD_is_bursty
        "inet::b",    // FIELD_payload_length
        "inet::b",    // FIELD_total_length
        "inet::b",    // FIELD_offset
        "bool",    // FIELD_is_v2_dropped_packet_header
        "uint16_t",    // FIELD_queue_occupancy
        "omnetpp::simtime_t",    // FIELD_time_packet_received_at_nic
        "omnetpp::simtime_t",    // FIELD_local_nic_rx_delay
        "omnetpp::simtime_t",    // FIELD_remote_queueing_time
        "omnetpp::simtime_t",    // FIELD_fabric_delay_time_sent_from_source
        "omnetpp::simtime_t",    // FIELD_time_packet_sent_from_src
        "bool",    // FIELD_is_deflected
    };
    return (field >= 0 && field < 25) ? fieldTypeStrings[field] : nullptr;
}

const char **EthernetMacHeaderDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldPropertyNames(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        case FIELD_cTag: {
            static const char *names[] = { "owned",  nullptr };
            return names;
        }
        case FIELD_sTag: {
            static const char *names[] = { "owned",  nullptr };
            return names;
        }
        default: return nullptr;
    }
}

const char *EthernetMacHeaderDescriptor::getFieldProperty(int field, const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldProperty(field, propertyname);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        case FIELD_cTag:
            if (!strcmp(propertyname, "owned")) return "";
            return nullptr;
        case FIELD_sTag:
            if (!strcmp(propertyname, "owned")) return "";
            return nullptr;
        default: return nullptr;
    }
}

int EthernetMacHeaderDescriptor::getFieldArraySize(void *object, int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldArraySize(object, field);
        field -= basedesc->getFieldCount();
    }
    EthernetMacHeader *pp = (EthernetMacHeader *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

const char *EthernetMacHeaderDescriptor::getFieldDynamicTypeString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldDynamicTypeString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    EthernetMacHeader *pp = (EthernetMacHeader *)object; (void)pp;
    switch (field) {
        case FIELD_cTag: { const Ieee8021qHeader * value = pp->getCTag(); return omnetpp::opp_typename(typeid(*value)); }
        case FIELD_sTag: { const Ieee8021qHeader * value = pp->getSTag(); return omnetpp::opp_typename(typeid(*value)); }
        default: return nullptr;
    }
}

std::string EthernetMacHeaderDescriptor::getFieldValueAsString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldValueAsString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    EthernetMacHeader *pp = (EthernetMacHeader *)object; (void)pp;
    switch (field) {
        case FIELD_dest: return pp->getDest().str();
        case FIELD_src: return pp->getSrc().str();
        case FIELD_cTag: {std::stringstream out; out << pp->getCTag(); return out.str();}
        case FIELD_sTag: {std::stringstream out; out << pp->getSTag(); return out.str();}
        case FIELD_typeOrLength: return long2string(pp->getTypeOrLength());
        case FIELD_hop_count: return long2string(pp->getHop_count());
        case FIELD_isFB: return bool2string(pp->isFB());
        case FIELD_allow_same_input_output: return bool2string(pp->getAllow_same_input_output());
        case FIELD_original_interface_id: return long2string(pp->getOriginal_interface_id());
        case FIELD_bouncedDistance: return long2string(pp->getBouncedDistance());
        case FIELD_maxBouncedDistance: return long2string(pp->getMaxBouncedDistance());
        case FIELD_bouncedHop: return long2string(pp->getBouncedHop());
        case FIELD_totalHopNum: return long2string(pp->getTotalHopNum());
        case FIELD_is_bursty: return bool2string(pp->getIs_bursty());
        case FIELD_payload_length: return unit2string(pp->getPayload_length());
        case FIELD_total_length: return unit2string(pp->getTotal_length());
        case FIELD_offset: return unit2string(pp->getOffset());
        case FIELD_is_v2_dropped_packet_header: return bool2string(pp->getIs_v2_dropped_packet_header());
        case FIELD_queue_occupancy: return ulong2string(pp->getQueue_occupancy());
        case FIELD_time_packet_received_at_nic: return simtime2string(pp->getTime_packet_received_at_nic());
        case FIELD_local_nic_rx_delay: return simtime2string(pp->getLocal_nic_rx_delay());
        case FIELD_remote_queueing_time: return simtime2string(pp->getRemote_queueing_time());
        case FIELD_fabric_delay_time_sent_from_source: return simtime2string(pp->getFabric_delay_time_sent_from_source());
        case FIELD_time_packet_sent_from_src: return simtime2string(pp->getTime_packet_sent_from_src());
        case FIELD_is_deflected: return bool2string(pp->getIs_deflected());
        default: return "";
    }
}

bool EthernetMacHeaderDescriptor::setFieldValueAsString(void *object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->setFieldValueAsString(object,field,i,value);
        field -= basedesc->getFieldCount();
    }
    EthernetMacHeader *pp = (EthernetMacHeader *)object; (void)pp;
    switch (field) {
        case FIELD_typeOrLength: pp->setTypeOrLength(string2long(value)); return true;
        case FIELD_hop_count: pp->setHop_count(string2long(value)); return true;
        case FIELD_isFB: pp->setIsFB(string2bool(value)); return true;
        case FIELD_allow_same_input_output: pp->setAllow_same_input_output(string2bool(value)); return true;
        case FIELD_original_interface_id: pp->setOriginal_interface_id(string2long(value)); return true;
        case FIELD_bouncedDistance: pp->setBouncedDistance(string2long(value)); return true;
        case FIELD_maxBouncedDistance: pp->setMaxBouncedDistance(string2long(value)); return true;
        case FIELD_bouncedHop: pp->setBouncedHop(string2long(value)); return true;
        case FIELD_totalHopNum: pp->setTotalHopNum(string2long(value)); return true;
        case FIELD_is_bursty: pp->setIs_bursty(string2bool(value)); return true;
        case FIELD_payload_length: pp->setPayload_length(b(string2long(value))); return true;
        case FIELD_total_length: pp->setTotal_length(b(string2long(value))); return true;
        case FIELD_offset: pp->setOffset(b(string2long(value))); return true;
        case FIELD_is_v2_dropped_packet_header: pp->setIs_v2_dropped_packet_header(string2bool(value)); return true;
        case FIELD_queue_occupancy: pp->setQueue_occupancy(string2ulong(value)); return true;
        case FIELD_is_deflected: pp->setIs_deflected(string2bool(value)); return true;
        default: return false;
    }
}

const char *EthernetMacHeaderDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructName(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        case FIELD_cTag: return omnetpp::opp_typename(typeid(Ieee8021qHeader));
        case FIELD_sTag: return omnetpp::opp_typename(typeid(Ieee8021qHeader));
        default: return nullptr;
    };
}

void *EthernetMacHeaderDescriptor::getFieldStructValuePointer(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructValuePointer(object, field, i);
        field -= basedesc->getFieldCount();
    }
    EthernetMacHeader *pp = (EthernetMacHeader *)object; (void)pp;
    switch (field) {
        case FIELD_dest: return toVoidPtr(&pp->getDest()); break;
        case FIELD_src: return toVoidPtr(&pp->getSrc()); break;
        case FIELD_cTag: return toVoidPtr(pp->getCTag()); break;
        case FIELD_sTag: return toVoidPtr(pp->getSTag()); break;
        default: return nullptr;
    }
}

Register_Class(EthernetControlFrame)

EthernetControlFrame::EthernetControlFrame() : ::inet::FieldsChunk()
{
}

EthernetControlFrame::EthernetControlFrame(const EthernetControlFrame& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

EthernetControlFrame::~EthernetControlFrame()
{
}

EthernetControlFrame& EthernetControlFrame::operator=(const EthernetControlFrame& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void EthernetControlFrame::copy(const EthernetControlFrame& other)
{
    this->opCode = other.opCode;
}

void EthernetControlFrame::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
    doParsimPacking(b,this->opCode);
}

void EthernetControlFrame::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
    doParsimUnpacking(b,this->opCode);
}

int EthernetControlFrame::getOpCode() const
{
    return this->opCode;
}

void EthernetControlFrame::setOpCode(int opCode)
{
    handleChange();
    this->opCode = opCode;
}

class EthernetControlFrameDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertynames;
    enum FieldConstants {
        FIELD_opCode,
    };
  public:
    EthernetControlFrameDescriptor();
    virtual ~EthernetControlFrameDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyname) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyname) const override;
    virtual int getFieldArraySize(void *object, int field) const override;

    virtual const char *getFieldDynamicTypeString(void *object, int field, int i) const override;
    virtual std::string getFieldValueAsString(void *object, int field, int i) const override;
    virtual bool setFieldValueAsString(void *object, int field, int i, const char *value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual void *getFieldStructValuePointer(void *object, int field, int i) const override;
};

Register_ClassDescriptor(EthernetControlFrameDescriptor)

EthernetControlFrameDescriptor::EthernetControlFrameDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(inet::EthernetControlFrame)), "inet::FieldsChunk")
{
    propertynames = nullptr;
}

EthernetControlFrameDescriptor::~EthernetControlFrameDescriptor()
{
    delete[] propertynames;
}

bool EthernetControlFrameDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<EthernetControlFrame *>(obj)!=nullptr;
}

const char **EthernetControlFrameDescriptor::getPropertyNames() const
{
    if (!propertynames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
        const char **basenames = basedesc ? basedesc->getPropertyNames() : nullptr;
        propertynames = mergeLists(basenames, names);
    }
    return propertynames;
}

const char *EthernetControlFrameDescriptor::getProperty(const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : nullptr;
}

int EthernetControlFrameDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 1+basedesc->getFieldCount() : 1;
}

unsigned int EthernetControlFrameDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeFlags(field);
        field -= basedesc->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_opCode
    };
    return (field >= 0 && field < 1) ? fieldTypeFlags[field] : 0;
}

const char *EthernetControlFrameDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldName(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldNames[] = {
        "opCode",
    };
    return (field >= 0 && field < 1) ? fieldNames[field] : nullptr;
}

int EthernetControlFrameDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount() : 0;
    if (fieldName[0] == 'o' && strcmp(fieldName, "opCode") == 0) return base+0;
    return basedesc ? basedesc->findField(fieldName) : -1;
}

const char *EthernetControlFrameDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeString(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_opCode
    };
    return (field >= 0 && field < 1) ? fieldTypeStrings[field] : nullptr;
}

const char **EthernetControlFrameDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldPropertyNames(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *EthernetControlFrameDescriptor::getFieldProperty(int field, const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldProperty(field, propertyname);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int EthernetControlFrameDescriptor::getFieldArraySize(void *object, int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldArraySize(object, field);
        field -= basedesc->getFieldCount();
    }
    EthernetControlFrame *pp = (EthernetControlFrame *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

const char *EthernetControlFrameDescriptor::getFieldDynamicTypeString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldDynamicTypeString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    EthernetControlFrame *pp = (EthernetControlFrame *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string EthernetControlFrameDescriptor::getFieldValueAsString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldValueAsString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    EthernetControlFrame *pp = (EthernetControlFrame *)object; (void)pp;
    switch (field) {
        case FIELD_opCode: return long2string(pp->getOpCode());
        default: return "";
    }
}

bool EthernetControlFrameDescriptor::setFieldValueAsString(void *object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->setFieldValueAsString(object,field,i,value);
        field -= basedesc->getFieldCount();
    }
    EthernetControlFrame *pp = (EthernetControlFrame *)object; (void)pp;
    switch (field) {
        case FIELD_opCode: pp->setOpCode(string2long(value)); return true;
        default: return false;
    }
}

const char *EthernetControlFrameDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructName(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

void *EthernetControlFrameDescriptor::getFieldStructValuePointer(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructValuePointer(object, field, i);
        field -= basedesc->getFieldCount();
    }
    EthernetControlFrame *pp = (EthernetControlFrame *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

Register_Class(EthernetPauseFrame)

EthernetPauseFrame::EthernetPauseFrame() : ::inet::EthernetControlFrame()
{
    this->setChunkLength(ETHER_PAUSE_COMMAND_BYTES);
    this->setOpCode(ETHERNET_CONTROL_PAUSE);

}

EthernetPauseFrame::EthernetPauseFrame(const EthernetPauseFrame& other) : ::inet::EthernetControlFrame(other)
{
    copy(other);
}

EthernetPauseFrame::~EthernetPauseFrame()
{
}

EthernetPauseFrame& EthernetPauseFrame::operator=(const EthernetPauseFrame& other)
{
    if (this == &other) return *this;
    ::inet::EthernetControlFrame::operator=(other);
    copy(other);
    return *this;
}

void EthernetPauseFrame::copy(const EthernetPauseFrame& other)
{
    this->pauseTime = other.pauseTime;
}

void EthernetPauseFrame::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::EthernetControlFrame::parsimPack(b);
    doParsimPacking(b,this->pauseTime);
}

void EthernetPauseFrame::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::EthernetControlFrame::parsimUnpack(b);
    doParsimUnpacking(b,this->pauseTime);
}

int EthernetPauseFrame::getPauseTime() const
{
    return this->pauseTime;
}

void EthernetPauseFrame::setPauseTime(int pauseTime)
{
    handleChange();
    this->pauseTime = pauseTime;
}

class EthernetPauseFrameDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertynames;
    enum FieldConstants {
        FIELD_pauseTime,
    };
  public:
    EthernetPauseFrameDescriptor();
    virtual ~EthernetPauseFrameDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyname) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyname) const override;
    virtual int getFieldArraySize(void *object, int field) const override;

    virtual const char *getFieldDynamicTypeString(void *object, int field, int i) const override;
    virtual std::string getFieldValueAsString(void *object, int field, int i) const override;
    virtual bool setFieldValueAsString(void *object, int field, int i, const char *value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual void *getFieldStructValuePointer(void *object, int field, int i) const override;
};

Register_ClassDescriptor(EthernetPauseFrameDescriptor)

EthernetPauseFrameDescriptor::EthernetPauseFrameDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(inet::EthernetPauseFrame)), "inet::EthernetControlFrame")
{
    propertynames = nullptr;
}

EthernetPauseFrameDescriptor::~EthernetPauseFrameDescriptor()
{
    delete[] propertynames;
}

bool EthernetPauseFrameDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<EthernetPauseFrame *>(obj)!=nullptr;
}

const char **EthernetPauseFrameDescriptor::getPropertyNames() const
{
    if (!propertynames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
        const char **basenames = basedesc ? basedesc->getPropertyNames() : nullptr;
        propertynames = mergeLists(basenames, names);
    }
    return propertynames;
}

const char *EthernetPauseFrameDescriptor::getProperty(const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : nullptr;
}

int EthernetPauseFrameDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 1+basedesc->getFieldCount() : 1;
}

unsigned int EthernetPauseFrameDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeFlags(field);
        field -= basedesc->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_pauseTime
    };
    return (field >= 0 && field < 1) ? fieldTypeFlags[field] : 0;
}

const char *EthernetPauseFrameDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldName(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldNames[] = {
        "pauseTime",
    };
    return (field >= 0 && field < 1) ? fieldNames[field] : nullptr;
}

int EthernetPauseFrameDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount() : 0;
    if (fieldName[0] == 'p' && strcmp(fieldName, "pauseTime") == 0) return base+0;
    return basedesc ? basedesc->findField(fieldName) : -1;
}

const char *EthernetPauseFrameDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeString(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_pauseTime
    };
    return (field >= 0 && field < 1) ? fieldTypeStrings[field] : nullptr;
}

const char **EthernetPauseFrameDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldPropertyNames(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *EthernetPauseFrameDescriptor::getFieldProperty(int field, const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldProperty(field, propertyname);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int EthernetPauseFrameDescriptor::getFieldArraySize(void *object, int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldArraySize(object, field);
        field -= basedesc->getFieldCount();
    }
    EthernetPauseFrame *pp = (EthernetPauseFrame *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

const char *EthernetPauseFrameDescriptor::getFieldDynamicTypeString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldDynamicTypeString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    EthernetPauseFrame *pp = (EthernetPauseFrame *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string EthernetPauseFrameDescriptor::getFieldValueAsString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldValueAsString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    EthernetPauseFrame *pp = (EthernetPauseFrame *)object; (void)pp;
    switch (field) {
        case FIELD_pauseTime: return long2string(pp->getPauseTime());
        default: return "";
    }
}

bool EthernetPauseFrameDescriptor::setFieldValueAsString(void *object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->setFieldValueAsString(object,field,i,value);
        field -= basedesc->getFieldCount();
    }
    EthernetPauseFrame *pp = (EthernetPauseFrame *)object; (void)pp;
    switch (field) {
        case FIELD_pauseTime: pp->setPauseTime(string2long(value)); return true;
        default: return false;
    }
}

const char *EthernetPauseFrameDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructName(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

void *EthernetPauseFrameDescriptor::getFieldStructValuePointer(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructValuePointer(object, field, i);
        field -= basedesc->getFieldCount();
    }
    EthernetPauseFrame *pp = (EthernetPauseFrame *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

Register_Class(EthernetPadding)

EthernetPadding::EthernetPadding() : ::inet::FieldsChunk()
{
}

EthernetPadding::EthernetPadding(const EthernetPadding& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

EthernetPadding::~EthernetPadding()
{
}

EthernetPadding& EthernetPadding::operator=(const EthernetPadding& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void EthernetPadding::copy(const EthernetPadding& other)
{
}

void EthernetPadding::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
}

void EthernetPadding::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
}

class EthernetPaddingDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertynames;
    enum FieldConstants {
    };
  public:
    EthernetPaddingDescriptor();
    virtual ~EthernetPaddingDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyname) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyname) const override;
    virtual int getFieldArraySize(void *object, int field) const override;

    virtual const char *getFieldDynamicTypeString(void *object, int field, int i) const override;
    virtual std::string getFieldValueAsString(void *object, int field, int i) const override;
    virtual bool setFieldValueAsString(void *object, int field, int i, const char *value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual void *getFieldStructValuePointer(void *object, int field, int i) const override;
};

Register_ClassDescriptor(EthernetPaddingDescriptor)

EthernetPaddingDescriptor::EthernetPaddingDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(inet::EthernetPadding)), "inet::FieldsChunk")
{
    propertynames = nullptr;
}

EthernetPaddingDescriptor::~EthernetPaddingDescriptor()
{
    delete[] propertynames;
}

bool EthernetPaddingDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<EthernetPadding *>(obj)!=nullptr;
}

const char **EthernetPaddingDescriptor::getPropertyNames() const
{
    if (!propertynames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
        const char **basenames = basedesc ? basedesc->getPropertyNames() : nullptr;
        propertynames = mergeLists(basenames, names);
    }
    return propertynames;
}

const char *EthernetPaddingDescriptor::getProperty(const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : nullptr;
}

int EthernetPaddingDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 0+basedesc->getFieldCount() : 0;
}

unsigned int EthernetPaddingDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeFlags(field);
        field -= basedesc->getFieldCount();
    }
    return 0;
}

const char *EthernetPaddingDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldName(field);
        field -= basedesc->getFieldCount();
    }
    return nullptr;
}

int EthernetPaddingDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->findField(fieldName) : -1;
}

const char *EthernetPaddingDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeString(field);
        field -= basedesc->getFieldCount();
    }
    return nullptr;
}

const char **EthernetPaddingDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldPropertyNames(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *EthernetPaddingDescriptor::getFieldProperty(int field, const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldProperty(field, propertyname);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int EthernetPaddingDescriptor::getFieldArraySize(void *object, int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldArraySize(object, field);
        field -= basedesc->getFieldCount();
    }
    EthernetPadding *pp = (EthernetPadding *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

const char *EthernetPaddingDescriptor::getFieldDynamicTypeString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldDynamicTypeString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    EthernetPadding *pp = (EthernetPadding *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string EthernetPaddingDescriptor::getFieldValueAsString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldValueAsString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    EthernetPadding *pp = (EthernetPadding *)object; (void)pp;
    switch (field) {
        default: return "";
    }
}

bool EthernetPaddingDescriptor::setFieldValueAsString(void *object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->setFieldValueAsString(object,field,i,value);
        field -= basedesc->getFieldCount();
    }
    EthernetPadding *pp = (EthernetPadding *)object; (void)pp;
    switch (field) {
        default: return false;
    }
}

const char *EthernetPaddingDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructName(field);
        field -= basedesc->getFieldCount();
    }
    return nullptr;
}

void *EthernetPaddingDescriptor::getFieldStructValuePointer(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructValuePointer(object, field, i);
        field -= basedesc->getFieldCount();
    }
    EthernetPadding *pp = (EthernetPadding *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

Register_Class(EthernetFcs)

EthernetFcs::EthernetFcs() : ::inet::FieldsChunk()
{
    this->setChunkLength(ETHER_FCS_BYTES);

}

EthernetFcs::EthernetFcs(const EthernetFcs& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

EthernetFcs::~EthernetFcs()
{
}

EthernetFcs& EthernetFcs::operator=(const EthernetFcs& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void EthernetFcs::copy(const EthernetFcs& other)
{
    this->fcs = other.fcs;
    this->fcsMode = other.fcsMode;
}

void EthernetFcs::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
    doParsimPacking(b,this->fcs);
    doParsimPacking(b,this->fcsMode);
}

void EthernetFcs::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
    doParsimUnpacking(b,this->fcs);
    doParsimUnpacking(b,this->fcsMode);
}

uint32_t EthernetFcs::getFcs() const
{
    return this->fcs;
}

void EthernetFcs::setFcs(uint32_t fcs)
{
    handleChange();
    this->fcs = fcs;
}

inet::FcsMode EthernetFcs::getFcsMode() const
{
    return this->fcsMode;
}

void EthernetFcs::setFcsMode(inet::FcsMode fcsMode)
{
    handleChange();
    this->fcsMode = fcsMode;
}

class EthernetFcsDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertynames;
    enum FieldConstants {
        FIELD_fcs,
        FIELD_fcsMode,
    };
  public:
    EthernetFcsDescriptor();
    virtual ~EthernetFcsDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyname) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyname) const override;
    virtual int getFieldArraySize(void *object, int field) const override;

    virtual const char *getFieldDynamicTypeString(void *object, int field, int i) const override;
    virtual std::string getFieldValueAsString(void *object, int field, int i) const override;
    virtual bool setFieldValueAsString(void *object, int field, int i, const char *value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual void *getFieldStructValuePointer(void *object, int field, int i) const override;
};

Register_ClassDescriptor(EthernetFcsDescriptor)

EthernetFcsDescriptor::EthernetFcsDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(inet::EthernetFcs)), "inet::FieldsChunk")
{
    propertynames = nullptr;
}

EthernetFcsDescriptor::~EthernetFcsDescriptor()
{
    delete[] propertynames;
}

bool EthernetFcsDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<EthernetFcs *>(obj)!=nullptr;
}

const char **EthernetFcsDescriptor::getPropertyNames() const
{
    if (!propertynames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
        const char **basenames = basedesc ? basedesc->getPropertyNames() : nullptr;
        propertynames = mergeLists(basenames, names);
    }
    return propertynames;
}

const char *EthernetFcsDescriptor::getProperty(const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : nullptr;
}

int EthernetFcsDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 2+basedesc->getFieldCount() : 2;
}

unsigned int EthernetFcsDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeFlags(field);
        field -= basedesc->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_fcs
        0,    // FIELD_fcsMode
    };
    return (field >= 0 && field < 2) ? fieldTypeFlags[field] : 0;
}

const char *EthernetFcsDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldName(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldNames[] = {
        "fcs",
        "fcsMode",
    };
    return (field >= 0 && field < 2) ? fieldNames[field] : nullptr;
}

int EthernetFcsDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount() : 0;
    if (fieldName[0] == 'f' && strcmp(fieldName, "fcs") == 0) return base+0;
    if (fieldName[0] == 'f' && strcmp(fieldName, "fcsMode") == 0) return base+1;
    return basedesc ? basedesc->findField(fieldName) : -1;
}

const char *EthernetFcsDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeString(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "uint32_t",    // FIELD_fcs
        "inet::FcsMode",    // FIELD_fcsMode
    };
    return (field >= 0 && field < 2) ? fieldTypeStrings[field] : nullptr;
}

const char **EthernetFcsDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldPropertyNames(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        case FIELD_fcsMode: {
            static const char *names[] = { "enum",  nullptr };
            return names;
        }
        default: return nullptr;
    }
}

const char *EthernetFcsDescriptor::getFieldProperty(int field, const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldProperty(field, propertyname);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        case FIELD_fcsMode:
            if (!strcmp(propertyname, "enum")) return "inet::FcsMode";
            return nullptr;
        default: return nullptr;
    }
}

int EthernetFcsDescriptor::getFieldArraySize(void *object, int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldArraySize(object, field);
        field -= basedesc->getFieldCount();
    }
    EthernetFcs *pp = (EthernetFcs *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

const char *EthernetFcsDescriptor::getFieldDynamicTypeString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldDynamicTypeString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    EthernetFcs *pp = (EthernetFcs *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string EthernetFcsDescriptor::getFieldValueAsString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldValueAsString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    EthernetFcs *pp = (EthernetFcs *)object; (void)pp;
    switch (field) {
        case FIELD_fcs: return ulong2string(pp->getFcs());
        case FIELD_fcsMode: return enum2string(pp->getFcsMode(), "inet::FcsMode");
        default: return "";
    }
}

bool EthernetFcsDescriptor::setFieldValueAsString(void *object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->setFieldValueAsString(object,field,i,value);
        field -= basedesc->getFieldCount();
    }
    EthernetFcs *pp = (EthernetFcs *)object; (void)pp;
    switch (field) {
        case FIELD_fcs: pp->setFcs(string2ulong(value)); return true;
        default: return false;
    }
}

const char *EthernetFcsDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructName(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

void *EthernetFcsDescriptor::getFieldStructValuePointer(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructValuePointer(object, field, i);
        field -= basedesc->getFieldCount();
    }
    EthernetFcs *pp = (EthernetFcs *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

} // namespace inet

